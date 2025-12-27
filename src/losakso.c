#include "z_asm.h"
#include "z_syscalls.h"
#include "z_utils.h"
#include "z_elf.h"

// Forward declarations for kernel functions
extern void* kmalloc(uint32_t size);
extern void kfree(void* ptr);
extern int fs_read_file(char* name, char* buffer, uint32_t max_size);
extern void putchar(char c);

#define NULL ((void*)0)

#define PAGE_SIZE	4096
#define ALIGN		(PAGE_SIZE - 1)
#define ROUND_PG(x)	(((x) + (ALIGN)) & ~(ALIGN))
#define TRUNC_PG(x)	((x) & ~(ALIGN))
#define PFLAGS(x)	((((x) & PF_R) ? PROT_READ : 0) | \
			 (((x) & PF_W) ? PROT_WRITE : 0) | \
			 (((x) & PF_X) ? PROT_EXEC : 0))
#define LOAD_ERR	((unsigned long)-1)

void z_fini(void)
{
	z_printf("Fini at work\n");
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

// Kernel space version - direct memory writes, no mmap
static unsigned long loadelf_anon(int fd, Elf_Ehdr *ehdr, Elf_Phdr *phdr)
{
	unsigned long minva, maxva;
	Elf_Phdr *iter;
	int dyn = ehdr->e_type == ET_DYN;
	unsigned char *p, *base = 0;

	minva = (unsigned long)-1;
	maxva = 0;
	
	for (iter = phdr; iter < &phdr[ehdr->e_phnum]; iter++) {
		if (iter->p_type != PT_LOAD)
			continue;
		if (iter->p_vaddr < minva)
			minva = iter->p_vaddr;
		if (iter->p_vaddr + iter->p_memsz > maxva)
			maxva = iter->p_vaddr + iter->p_memsz;
	}

	minva = TRUNC_PG(minva);
	maxva = ROUND_PG(maxva);

	/* For static ELF, use original addresses. For dynamic, use base. */
	if (dyn) {
		base = (unsigned char *)0x400000;  // USER_CODE_START
	}

	/* Load each segment directly to memory. */
	for (iter = phdr; iter < &phdr[ehdr->e_phnum]; iter++) {
		unsigned long off, start;
		if (iter->p_type != PT_LOAD)
			continue;
		off = iter->p_vaddr & ALIGN;
		start = dyn ? (unsigned long)base : 0;
		start += TRUNC_PG(iter->p_vaddr);

		p = (unsigned char *)start;
		if (z_lseek(fd, iter->p_offset, SEEK_SET) < 0)
			goto err;
		if (z_read(fd, p + off, iter->p_filesz) !=
				(ssize_t)iter->p_filesz)
			goto err;
		/* Zero BSS */
		if (iter->p_memsz > iter->p_filesz) {
			z_memset(p + off + iter->p_filesz, 0,
				 iter->p_memsz - iter->p_filesz);
		}
	}

	return (unsigned long)base;
err:
	return LOAD_ERR;
}

#define Z_PROG		0
#define Z_INTERP	1

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
	if (argc < 2)
		z_errx(1, "no input file");
	file = argv[1];

	for (i = 0;; i++, ehdr++) {
		/* Open file, read and than check ELF header.*/
		if ((fd = z_open(file, O_RDONLY)) < 0)
			z_errx(1, "can't open %s", file);
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

		/* Set the entry point, if the file is dynamic than add bias. */
		entry[i] = ehdr->e_entry + (ehdr->e_type == ET_DYN ? base[i] : 0);
		{
			unsigned char *entry_bytes = (unsigned char *)entry[i];
			z_printf("ELF[%d] entry %x first16:", i, (unsigned int)entry[i]);
			for (int dump = 0; dump < 16; dump++) {
				z_printf(" %02x", entry_bytes[dump]);
			}
			z_printf("\n");
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

	/* Shift argv, env and av. */
	z_memcpy(&argv[0], &argv[1],
		 (unsigned long)av - (unsigned long)&argv[1]);
	/* SP points to argc. */
	(*sp)--;

	{
		unsigned long target = (elf_interp ? entry[Z_INTERP] : entry[Z_PROG]);
		z_printf("Jumping to entry %x with sp ptr %x value0 %x\n",
			 (unsigned int)target,
			 (unsigned int)sp,
			 (unsigned int)*sp);
		z_trampo((void (*)(void))target, sp, z_fini);
	}
	/* Should not reach. */
	z_exit(0);
}

