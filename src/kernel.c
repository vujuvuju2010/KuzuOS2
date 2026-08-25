// Kendi typedef'lerimiz
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

// VGA buffer
#define VGA_BUFFER 0xB8000
unsigned int gigawigga = 0x00000000; // a variable used to hlt the system at hlt.c and name came from a reel i saw lmao
// Global command arguments storage
char kernel_cmd_args[256] = {0};

// Global current working directory
char kernel_cwd[256] = "/";

#define USB_CMD 0X20 // for reset
#define USB_STS 0X24 // for checking if reset is done

// Header'ları dahil et
#include "memory.h"
#include "pmm.h"
#include "vmm.h"
#include "interrupts.h"
#include "keyboard.h"
#include "irq.h"
#include "process.h"
#include "filesystem.h"
#include "shell.h"
#include "vga.h"
#include "banner.h"
#include "syscall.h"
#include "internet/netstack.h"
#include "usb.h" // usb time 
#include "keyboardusb.h"
#include "keymap_loader.h"
#include "z_utils.h" // for za printf for bar0 and other ints
#include "tty.h" // tty
#include "kuzulib/fs/vfs.h" // for vfs
#include "fatfs/ff.h"
// FPU initialization
extern void fpu_init(void);

// Multiboot2 header (sadece multiboot için, framebuffer yok)
#define MULTIBOOT2_HEADER_MAGIC 0xE85250D6
#define MULTIBOOT2_HEADER_ARCHITECTURE 0
#define MULTIBOOT2_HEADER_LENGTH 16
#define MULTIBOOT2_HEADER_CHECKSUM -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT2_HEADER_ARCHITECTURE + MULTIBOOT2_HEADER_LENGTH)
__attribute__((section(".multiboot2.header")))
unsigned int multiboot2_header[] = {
    MULTIBOOT2_HEADER_MAGIC,
    MULTIBOOT2_HEADER_ARCHITECTURE,
    MULTIBOOT2_HEADER_LENGTH,
    MULTIBOOT2_HEADER_CHECKSUM
};

static uint64_t detect_available_memory(uint32_t mb_addr) {
   
    
    if (mb_addr == 0) return 128 * 1024 * 1024;  // Fallback to 128MB if invalid
    
    uint32_t* tag_ptr = (uint32_t*)(uint64_t)(mb_addr + 8);
    uint32_t max_iterations = 1000;  
    
    while (max_iterations-- > 0) {
        uint32_t tag_type = tag_ptr[0];
        uint32_t tag_size = tag_ptr[1];
        
        if (tag_type == 0 && tag_size == 0) {
            break;
        }
        
        if (tag_type == 4 && tag_size >= 16) {
            uint32_t mem_lower = tag_ptr[2];  // kilobytes below 1MB
            uint32_t mem_upper = tag_ptr[3];  
            uint64_t total = ((uint64_t)(mem_lower + mem_upper) * 1024);
            if (total > 0) return total;
        }
        
        // Type 6: memory map (newer format)
        if (tag_type == 6 && tag_size >= 16) {
            uint32_t entry_size = tag_ptr[2];
            uint32_t entry_version = tag_ptr[3];
            uint64_t highest_addr = 0;
            
            uint8_t* entry_base = (uint8_t*)(tag_ptr + 4);
            uint32_t entries_size = tag_size - 16;
            uint32_t offset = 0;
            
            while (offset < entries_size && entry_size > 0) {
                uint64_t* addr = (uint64_t*)(entry_base + offset);
                uint64_t* len = (uint64_t*)(entry_base + offset + 8);
                uint32_t* type = (uint32_t*)(entry_base + offset + 16);
                
                if (*type == 1) {  // Available memory
                    uint64_t end = *addr + *len;
                    if (end > highest_addr) highest_addr = end;
                }
                
                offset += entry_size;
            }
            
            if (highest_addr > 0) return highest_addr;
        }
        
        // Move to next tag (8-byte aligned)
        if (tag_size == 0) break;
        uint32_t next_offset = (tag_size + 7) & ~7;
        tag_ptr = (uint32_t*)((uint8_t*)tag_ptr + next_offset);
    }
    
    return 128 * 1024 * 1024;  // Default fallback to 128MB
}

extern void gdt_init();



void delay(int ms) {
    for (volatile int i = 0; i < ms * 7000; i++) {
        __asm__ volatile("nop");
    }
}

void kernel_main(uint32_t mb_magic, uint32_t mb_addr) {
    gdt_init();
    interrupts_init();
    fpu_init();
    vga_init(mb_magic, mb_addr);
    clear_screen();

    print_color("KuzuOS2 is bootinggg thou shall be patientt\n\n", VGA_COLOR_LIGHT_GREY);
    if (gigawigga == 0) {
        print_color("Gigawigga is zero yayyyy\n", VGA_COLOR_LIGHT_GREEN);
    } else {
        print_color("SOMEONE CHANGED GIGAWIGGA HOLY HELL imma hlt yo system go and chenck gigawigga\n", VGA_COLOR_RED);
        __asm__ volatile("hlt");

    }
    
    // Initialize Phase 2 memory management (PMM + VMM)
    print("[ "); print_color("..", VGA_COLOR_YELLOW); print(" ] Initializing PMM...   "); delay(200);
    pmm_init(mb_addr);
    print_color("OK\n", VGA_COLOR_LIGHT_GREEN);
    
    print("[ "); print_color("..", VGA_COLOR_YELLOW); print(" ] Initializing VMM...   "); delay(200);
    vmm_init();
    print_color("OK\n", VGA_COLOR_LIGHT_GREEN);
    
    print("[ "); print_color("..", VGA_COLOR_YELLOW); print(" ] Switching to new page tables... "); delay(200);
    vmm_switch_to_new_tables();
    print_color("OK\n", VGA_COLOR_LIGHT_GREEN);
    
    print("[ "); print_color("..", VGA_COLOR_YELLOW); print(" ] Mapping framebuffer... "); delay(200);
    vmm_map_framebuffer();
    print_color("OK\n", VGA_COLOR_LIGHT_GREEN);
    
    // Detect available memory from multiboot2
    uint64_t available_mem = detect_available_memory(mb_addr);
    print("[ "); print_color("..", VGA_COLOR_YELLOW); print(" ] RAM check:            "); delay(350);
    
    // Convert to MB for display
    char mem_str[32];
    uint64_t mb_val = available_mem >> 20;
    int mem_len = 0;
    uint64_t temp = mb_val;
    if (temp == 0) { mem_str[0] = '0'; mem_len = 1; }
    else {
        char rev[32]; int rlen = 0;
        while (temp > 0) { rev[rlen++] = '0' + (temp % 10); temp /= 10; }
        while (rlen > 0) mem_str[mem_len++] = rev[--rlen];
    }
    // Append " MB detected\n"
    const char* suffix = " MB detected\n";
    int i = 0;
    while (suffix[i]) { mem_str[mem_len + i] = suffix[i]; i++; }
    mem_str[mem_len + i] = '\0';
    print_color(mem_str, VGA_COLOR_LIGHT_GREEN);
    delay(500);

   
    // FS go brrrrrrrrrr
    print_color("FS go brrrrrrrrrr\n", VGA_COLOR_LIGHT_GREEN);
    fs_init();
    
    // process go brrrrr
    process_init();
    
    // syscall go brrrr
    syscall_init();

    // i hope the netwrok goes brrrrrrrrrr
    print_color("Netwrok go brrrrrrrrrr\n", VGA_COLOR_LIGHT_GREEN);
    net_init();
    
    // USBBB
    print_color("time for usb brrrrrrrrrr\n", VGA_COLOR_LIGHT_GREEN);
    usb_init();
    
    // USB boot check (must run AFTER usb_init)
    print_color("Checking for USB boot...\n", VGA_COLOR_LIGHT_CYAN);
    fs_late_init();

    // ECHI WAKTIII
    /*
    usbaddr(1);
    usb_setup_t* setup2 = (usb_setup_t*)dma_alloc(sizeof(usb_setup_t)); // thing
    unsigned char* desc2 = (unsigned char*)dma_alloc(18);
    setup2->bmreq = 0x80;
    setup2->breq      = 0x06;
    setup2->wval        = 0x0100;
    setup2->widx        = 0;
    setup2->wlen       = 18;
    
    for(volatile int i = 0; i < 1000000; i++);
    int r = ehci_control(1, setup2, desc2, 18, 1); // adres suan 1
    if(r == 0) z_printf("ADRESS 1 VALLA 1 vendor=0x%x\n", *(unsigned short*)(desc2+8));
    else z_printf("adress 1 unfortunatley sharted itself gng :(");

    usbkeyboard();
    load_us_keymap(); // load keymap so usbkbd_convert() works

   // loop will come here for usb kayboard
  usbkbd_poll();
    */
    
    // timme for the VFS init yknow 
    static FATFS usb_fs;
    
    
    print_color("im doing the VFS inittt\n", VGA_COLOR_GREEN);
    vfs_init(); // initilize the vfs
    vfs_mount("/",  VFS_FS_RAMFS, 0); 
    print_color("IM MOUNTING THE RAMFS\n", VGA_COLOR_GREEN);
    f_mount(&usb_fs, "1:", 1);
    fs_create_directory_raw("/usb"); // rawdawg 
    vfs_mount("/usb", VFS_FS_FAT32, &usb_fs);
    print_color("VFS DONNEEEEE\n", VGA_COLOR_GREEN);
    
    // Load US keymap after filesystem is ready
    print_color("Loading US keymap...\n", VGA_COLOR_GREEN);
    load_us_keymap();
    
    
    print_color("\nKuzuOS successfully started!\n", VGA_COLOR_CYAN);
    print_color("Type 'help' for commands.\n\n", VGA_COLOR_LIGHT_GREY);
     


     uint64_t current_rsp;
    __asm__ volatile("mov %%rsp, %0" : "=r"(current_rsp));
    extern void tss_set_kernel_stack(uint64_t);
    tss_set_kernel_stack(current_rsp);

    // Create shell as a process
    extern uint32_t process_create_shell();
    extern struct process* process_find(uint32_t pid);
    extern void context_restore(struct cpu_context* ctx);
    extern struct process* current_process;
    extern struct process* shell_process;
    
    uint32_t shell_pid = process_create_shell();
    print_color("[DEBUG] shell_pid=", VGA_COLOR_LIGHT_CYAN);
    char pidbuf[16];
    int pidval = shell_pid;
    int pidpos = 0;
    if (pidval == 0) {
        pidbuf[pidpos++] = '0';
    } else {
        char rev[16];
        int rp = 0;
        while (pidval > 0 && rp < 15) {
            rev[rp++] = '0' + (pidval % 10);
            pidval /= 10;
        }
        while (rp > 0) {
            pidbuf[pidpos++] = rev[--rp];
        }
    }
    pidbuf[pidpos] = '\0';
    print_color(pidbuf, VGA_COLOR_LIGHT_CYAN);
    print_color("\n", VGA_COLOR_LIGHT_CYAN);
    
    if (shell_pid) {
        shell_process = process_find(shell_pid);
        print_color("[DEBUG] shell_process found\n", VGA_COLOR_LIGHT_CYAN);
        if (shell_process) {
            // Switch to shell process
            print_color("[DEBUG] About to restore shell context...\n", VGA_COLOR_LIGHT_CYAN);
            shell_process->state = PROCESS_RUNNING;
            current_process = shell_process;
            print_color("[DEBUG] Calling context_restore now\n", VGA_COLOR_LIGHT_CYAN);
            context_restore(&shell_process->context);
            print_color("[DEBUG] ERROR: context_restore returned!\n", VGA_COLOR_RED);
        } else {
            // Fallback: run shell directly if process creation failed
            print_color("Failed to create shell process, running directly...\n", VGA_COLOR_RED);
            shell_run();
        }
    } else {
        // Fallback: run shell directly if process creation failed
        print_color("ur fucked gng", VGA_COLOR_RED);
        shell_run();
    }
    

    while (1) {
        net_poll();
        __asm__ volatile("hlt");
    }
} 




