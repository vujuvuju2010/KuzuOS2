#include "z_asm.h"
#include "z_syscalls.h"
#include "z_utils.h"
#include "z_elf.h"
#include "process.h"

// Forward declarations for kernel functions
extern void* kmalloc(uint32_t size);
extern void kfree(void* ptr);
extern int fs_read_file(char* name, char* buffer, uint32_t max_size);
extern void putchar(char c);

#define NULL ((void*)0)

#define PAGE_SIZE       4096
#define ALIGN           (PAGE_SIZE - 1)
#define ROUND_PG(x)     (((x) + (ALIGN)) & ~(ALIGN))
#define TRUNC_PG(x)     ((x) & ~(ALIGN))
#define PFLAGS(x)       ((((x) & PF_R) ? PROT_READ : 0) | \
                         (((x) & PF_W) ? PROT_WRITE : 0) | \
                         (((x) & PF_X) ? PROT_EXEC : 0))
#define LOAD_ERR        ((unsigned long)-1)

void z_fini(void)
{
}

static int check_ehdr(Elf_Ehdr *ehdr)
{
        unsigned char *e_ident = ehdr->e_ident;
        return (e_ident[EI_MAG0] != ELFMAG0 || e_ident[EI_MAG1] != ELFMAG1 ||
                e_ident[EI_MAG2] != ELFMAG2 || e_ident[EI_MAG3] != ELFMAG3 ||
                e_ident[EI_CLASS] != ELFCLASS ||
                e_ident[EI_VERSION] != EV_CURRENT ||
                (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN)) ? 0 : 1;
}

// User space address range limits (prevents loading into kernel memory)
// These limits are tuned for KuzuOS5's user space allocation
// User programs are typically loaded in the 0x00400000 range
// Kernel starts around 0xC0000000 (standard x86 split)
#define USER_SPACE_START        0x00000000  // Allow from low memory (ET_DYN may use any address)
#define USER_SPACE_END          0xB0000000  // End of user space (well below 0xC0000000 kernel start)

// Kernel space version - handles both ET_EXEC and ET_DYN
static unsigned long loadelf_anon(int fd, Elf_Ehdr *ehdr, Elf_Phdr *phdr)
{
        unsigned long minva, maxva, size;
        Elf_Phdr *iter;
        unsigned char *base = 0;

        minva = (unsigned long)-1;
        maxva = 0;

        // Find the min and max virtual addresses
        for (iter = phdr; iter < &phdr[ehdr->e_phnum]; iter++) {
                if (iter->p_type != PT_LOAD)
                        continue;
                
                // Validate segment address is in user space
                if (iter->p_vaddr < USER_SPACE_START) {
                        goto err;
                }
                if (iter->p_vaddr + iter->p_memsz > USER_SPACE_END) {
                        goto err;
                }
                
                if (iter->p_vaddr < minva)
                        minva = iter->p_vaddr;
                if (iter->p_vaddr + iter->p_memsz > maxva)
                        maxva = iter->p_vaddr + iter->p_memsz;
        }

        minva = TRUNC_PG(minva);
        maxva = ROUND_PG(maxva);
        size = maxva - minva;



        // For ET_EXEC: Load directly at the virtual addresses (identity mapped)
        // For ET_DYN: Allocate memory and load there
        if (ehdr->e_type == ET_EXEC) {
                // Load at exact virtual addresses - need to allocate physical pages
                // and map them to the requested virtual addresses
                extern int vmm_map_page(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags);
                extern uint64_t pmm_alloc_frame(void);
                
                // Allocate and map pages for the entire region
                for (uint64_t vaddr = minva; vaddr < maxva; vaddr += PAGE_SIZE) {
                        uint64_t phys_frame = pmm_alloc_frame();
                        if (phys_frame == 0) {
                                goto err;
                        }
                        // Map as user-accessible, writable, present
                        // PAGE_PRESENT=1, PAGE_WRITABLE=2, PAGE_USER=4
                        if (vmm_map_page(vaddr, phys_frame, 0x7) != 0) {
                                goto err;
                        }
                }
                
                base = (unsigned char *)minva;
                z_memset(base, 0, size);
        } else {
                // ET_DYN - allocate memory
                base = (unsigned char *)kmalloc(size);
                if (!base) {
                        goto err;
                }
                z_memset(base, 0, size);
        }

        // Load each segment
        for (iter = phdr; iter < &phdr[ehdr->e_phnum]; iter++) {
                unsigned char *dest;
                
                if (iter->p_type != PT_LOAD)
                        continue;

                if (ehdr->e_type == ET_EXEC) {
                        // Load at exact virtual address
                        dest = (unsigned char *)iter->p_vaddr;
                } else {
                        // Load relative to allocated base
                        unsigned long offset_in_base = iter->p_vaddr - minva;
                        dest = base + offset_in_base;
                }
                
                if (z_lseek(fd, iter->p_offset, SEEK_SET) < 0) {
                        goto err_free;
                }
                if (z_read(fd, dest, iter->p_filesz) != (ssize_t)iter->p_filesz) {
                        goto err_free;
                }
                
                // Zero BSS
                if (iter->p_memsz > iter->p_filesz) {
                        z_memset(dest + iter->p_filesz, 0,
                                 iter->p_memsz - iter->p_filesz);
                }
        }

        // For ET_EXEC, return the minva (where it was loaded)
        // For ET_DYN, return the allocated base
        return (unsigned long)base;

err_free:
        if (ehdr->e_type == ET_DYN && base)
                kfree(base);
err:
        return LOAD_ERR;
}

#define Z_PROG          0
#define Z_INTERP        1

void z_entry(unsigned long *sp, void (*fini)(void))
{
        Elf_Ehdr ehdrs[2], *ehdr = ehdrs;
        Elf_Phdr *phdr, *iter;
        Elf_auxv_t *av;
        char **argv, **env, **p, *elf_interp = NULL;
        unsigned long base[2], entry[2];
        const char *file;
        ssize_t sz;
        int argc, fd, i;

        (void)fini;

        argc = (int)*(sp);
        argv = (char **)(sp + 1);
        env = p = (char **)&argv[argc + 1];
        while (*p++ != NULL)
                ;
        av = (void *)p;

        (void)env;

        /* Kernel execve passes the binary path via current_process, not argv[1]. */
        if (current_process && current_process->elf_filename_ptr)
                file = (const char *)current_process->elf_filename_ptr;
        else {
                if (argc < 2)
                        z_errx(1, "no input file");
                file = argv[1];
        }

        for (i = 0;; i++, ehdr++) {
                /* Open file, read and than check ELF header.*/
                if ((fd = z_open(file, O_RDONLY)) < 0) {
                        // If this is the interpreter and we can't find it, just skip it
                        // (allows running dynamically-linked binaries even without interpreter)
                        if (elf_interp != NULL && file == elf_interp) {
        
                                break;
                        }
                        z_errx(1, "can't open %s", file);
                }
                if (z_read(fd, ehdr, sizeof(*ehdr)) != sizeof(*ehdr))
                        z_errx(1, "can't read ELF header %s", file);
                if (!check_ehdr(ehdr))
                        z_errx(1, "bogus ELF header %s", file);



                /* Read the program header. */
                sz = ehdr->e_phnum * sizeof(Elf_Phdr);
                phdr = z_alloca(sz);
                if (z_lseek(fd, ehdr->e_phoff, SEEK_SET) < 0)
                        z_errx(1, "can't lseek to program header %s", file);
                if (z_read(fd, phdr, sz) != sz)
                        z_errx(1, "can't read program header %s", file);
                
                /* Time to load ELF. */
                if ((base[i] = loadelf_anon(fd, ehdr, phdr)) == LOAD_ERR)
                        z_errx(1, "can't load ELF %s", file);

                /* Calculate entry point */
                if (ehdr->e_type == ET_EXEC) {
                        // For ET_EXEC, entry is an absolute virtual address
                        entry[i] = ehdr->e_entry;
                } else {
                        // For ET_DYN, entry is relative to base
                        entry[i] = base[i] + ehdr->e_entry;
                }
                

                
                /* The second round, we've loaded ELF interp. */
                if (file == elf_interp) {
                        z_close(fd);
                        break;
                }

                for (iter = phdr; iter < &phdr[ehdr->e_phnum]; iter++) {
                        if (iter->p_type != PT_INTERP)
                                continue;
                        elf_interp = z_alloca(iter->p_filesz);
                        if (z_lseek(fd, iter->p_offset, SEEK_SET) < 0)
                                z_errx(1, "can't lseek interp segment");
                        if (z_read(fd, elf_interp, iter->p_filesz) !=
                                        (ssize_t)iter->p_filesz)
                                z_errx(1, "can't read interp segment");
                        if (elf_interp[iter->p_filesz - 1] != '\0')
                                z_errx(1, "bogus interp path");
                        file = elf_interp;
                }

                z_close(fd);
                /* Looks like the ELF is static -- leave the loop. */
                if (elf_interp == NULL)
                        break;
        }

        /* Reassign some vectors that are important for
         * the dynamic linker and for lib C. */
#define AVSET(t, v, expr) case (t): (v)->a_un.a_val = (expr); break
        while (av->a_type != AT_NULL) {
                switch (av->a_type) {
                AVSET(AT_PHDR, av, base[Z_PROG] + ehdrs[Z_PROG].e_phoff);
                AVSET(AT_PHNUM, av, ehdrs[Z_PROG].e_phnum);
                AVSET(AT_PHENT, av, ehdrs[Z_PROG].e_phentsize);
                AVSET(AT_ENTRY, av, entry[Z_PROG]);
                AVSET(AT_EXECFN, av, (unsigned long)argv[1]);
                AVSET(AT_BASE, av, elf_interp ?
                                base[Z_INTERP] : av->a_un.a_val);
                }
                ++av;
        }
#undef AVSET
        ++av;

        /* Legacy z_start layout: argv[0]=prog, argv[1]=path — drop argv[0].
         * Kernel execve layout: argv is already [prog][args...]; do not shift.
         * For KuzuOS2 service manager, we keep argv as-is since shell passes it correctly. */
        /* Disabled argv shift - loader_kernel.c already sets up argv correctly */
        /*
        if (!(current_process && current_process->elf_filename_ptr)) {
                z_memcpy(&argv[0], &argv[1],
                         (unsigned long)av - (unsigned long)&argv[1]);
                (*sp)--;
        }
        */

        {
                unsigned long target = (elf_interp ? entry[Z_INTERP] : entry[Z_PROG]);
                extern void z_printf(const char* fmt, ...);
                z_printf("[z_entry] jumping to entry=0x%x sp=0x%x\n", (unsigned int)target, (unsigned int)sp);
                z_trampo((void (*)(void))target, sp, z_fini);
        }
        /* Should not reach. */
        z_exit(0);
}
