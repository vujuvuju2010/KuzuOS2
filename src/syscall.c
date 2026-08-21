// MAIN SYSCALL CODE FOR KUZUOS2 its mostly claude dw i ofc checked the code but i was too lazy to delete the commands plus they get helpfull
#include "syscall.h"
#include "vga.h"
#include "filesystem.h"
#include "memory.h"
#include "process.h"
#include "internet/net.h"
#include "internet/tcp.h"
#include "internet/arp.h"
#include "internet/ip.h"
#include "kuzulib/fs/vfs.h"   
#include "lsusb.h"
#include "usb.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

extern int gigawigga;
extern int icmp_ping(ip_addr_t dst_ip);
extern int icmp_is_reply_received(void);
extern void icmp_clear_reply_flag(void);
extern void icmp_clear_pending_on_timeout(void);
extern int icmp_got_reply;

/* ── tiny int printer ─────────────────────────────────────────── */

static void print_int_helper(int n){
    if(n < 0){ putchar('-'); n = -n; }
    if(n == 0){ putchar('0'); return; }
    char buf[12]; int i = 0;
    while(n > 0){ buf[i++] = '0' + (n % 10); n /= 10; }
    while(i > 0) putchar(buf[--i]);
}

/* ═══════════════════════════════════════════════════════════════
   syscall_init — nothing to initialise for fds anymore;
   vfs_init() must be called earlier in kernel_main.
   We keep this function so nothing breaks at the call site.
   ═══════════════════════════════════════════════════════════════ */

void syscall_init(void){
    print_color("[Syscall] VFS-backed syscall table ready\n", VGA_COLOR_LIGHT_GREEN);
}

/* ═══════════════════════════════════════════════════════════════
   Main dispatcher
   ═══════════════════════════════════════════════════════════════ */

int32_t handle_syscall(uint64_t syscall_num,
                       uint64_t arg1, uint64_t arg2, uint64_t arg3,
                       uint64_t arg4, uint64_t arg5, uint64_t arg6)
{
    (void)arg4; (void)arg5; (void)arg6;

    switch(syscall_num){

        /* ── SYS_WRITE ───────────────────────────────────────── */
        case SYS_WRITE: {
            int fd           = (int)arg1;
            const char *buf  = (const char *)arg2;
            uint64_t count   = arg3;
            /* sys_write handles fd 1/2 as console internally */
            return sys_write(fd, buf, count);
        }

        /* ── SYS_READ ────────────────────────────────────────── */
        case SYS_READ: {
            int fd       = (int)arg1;
            char *buf    = (char *)arg2;
            uint64_t len = arg3;
            /* sys_read handles fd 0 as console internally */
            return sys_read(fd, buf, len);
        }

        /* ── SYS_OPEN ────────────────────────────────────────── */
        case SYS_OPEN: {
            const char *path = (const char *)arg1;
            int flags        = (int)arg2;

            /* map posix O_* flags to VFS flags */
            int vflags = 0;
            if((flags & 3) == 0 || (flags & 3) == 2) vflags |= VFS_FLAG_READ;
            if((flags & 3) == 1 || (flags & 3) == 2) vflags |= VFS_FLAG_WRITE;

            return sys_open(path, vflags);
        }

        /* ── SYS_OPENAT ──────────────────────────────────────── */
        case SYS_OPENAT: {
            /* arg1 = dirfd (we ignore it, treat as AT_FDCWD) */
            const char *filename = (const char *)arg2;
            int flags            = (int)arg3;

            /* resolve relative paths using kernel_cwd */
            char resolved[256];
            if(filename && filename[0] != '/'){
                extern char kernel_cwd[256];
                int i = 0;
                while(kernel_cwd[i] && i < 254) { resolved[i] = kernel_cwd[i]; i++; }
                if(i > 1 && resolved[i-1] != '/') resolved[i++] = '/';
                int j = 0;
                while(filename[j] && i < 255) resolved[i++] = filename[j++];
                resolved[i] = '\0';
            } else {
                int i = 0;
                while(filename && filename[i] && i < 255){ resolved[i] = filename[i]; i++; }
                resolved[i] = '\0';
            }

            int vflags = 0;
            if((flags & 3) == 0 || (flags & 3) == 2) vflags |= VFS_FLAG_READ;
            if((flags & 3) == 1 || (flags & 3) == 2) vflags |= VFS_FLAG_WRITE;

            return sys_open(resolved, vflags);
        }

        /* ── SYS_CLOSE ───────────────────────────────────────── */
        case SYS_CLOSE: {
            return sys_close((int)arg1);
        }

        /* ── SYS_LSEEK ───────────────────────────────────────── */
        case SYS_LSEEK: {
            int fd        = (int)arg1;
            int32_t off   = (int32_t)arg2;
            int whence    = (int)arg3;
            return sys_lseek(fd, off, whence);
        }

        /* ── SYS_STAT ────────────────────────────────────────── */
        case SYS_STAT:
        case SYS_FSTAT: {
            const char *path  = (const char *)arg1;
            uint32_t *sz_out  = (uint32_t *)arg2;
            int *isdir_out    = (int *)arg3;
            return sys_stat(path, sz_out, isdir_out);
        }

        /* ── SYS_GETDENTS / SYS_READDIR (141) ───────────────── */
        case SYS_GETDENTS: {
            int fd           = (int)arg1;
            uint32_t index   = arg2;
            char *name_out   = (char *)arg3;
            int *isdir_out   = (int *)arg4;
            return sys_readdir(fd, index, name_out, isdir_out);
        }

        /* ── WIGGA_UPDATE ────────────────────────────────────── */
        case WIGGA_UPDATE: {
            gigawigga = 0x3169666;
            /* fall through intentional — same as old code */
        }

        /* ── SYS_MKDIR ───────────────────────────────────────── */
        case SYS_MKDIR: {
            const char *path = (const char *)arg1;
            extern int fs_create_directory(char *path);
            return fs_create_directory((char *)path);
        }

        /* ── SYS_REBOOT ──────────────────────────────────────── */
        case SYS_REBOOT: {
            print_color("\nSystem rebooting...\n", VGA_COLOR_LIGHT_GREY);
            __asm__ volatile("cli");
            __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0xFE), "Nd"(0x64));
            while(1) __asm__ volatile("hlt");
            return 0;
        }

        /* ── GET_WIGGA ───────────────────────────────────────── */
        case GET_WIGGA: {
            return gigawigga;
        }

        /* ── SYS_HLT ─────────────────────────────────────────── */
        case SYS_HLT: {
            __asm__ volatile("hlt");
            return 0;
        }

        /* ── SYS_LOADKEYMAP ──────────────────────────────────── */
        case 251: {
            const char *path = (const char *)arg1;
            extern int keymap_load(const char* path);
            return keymap_load(path);
        }

        /* ── SYS_GETCWD ──────────────────────────────────────── */
        case SYS_GETCWD: {
            char *buf = (char *)arg1;
            uint64_t size = arg2;
            extern char kernel_cwd[256];
            if (!buf || size == 0) return -1;
            uint64_t i = 0;
            while (i < size - 1 && kernel_cwd[i]) {
                buf[i] = kernel_cwd[i];
                i++;
            }
            buf[i] = '\0';
            return (int32_t)i;
        }

        /* ── SYS_CHDIR ───────────────────────────────────────── */
        case 12: {
            const char *path = (const char *)arg1;
            extern int fs_chdir(char *path);
            return fs_chdir((char *)path);
        }

        /* ── SYS_EXIT / SYS_EXIT_GROUP ───────────────────────── */
        case SYS_EXIT:
        case SYS_EXIT_GROUP: {
            int code = (int)arg1;

            /* close every open vfs fd for this process */
            sys_close_all();

            print_color("\n[Program exited with code ", VGA_COLOR_LIGHT_GREY);
            char buf[16]; int pos = 0;
            if(code == 0){
                buf[pos++] = '0';
            } else {
                if(code < 0){ buf[pos++] = '-'; code = -code; }
                char tmp[16]; int tp = 0;
                while(code > 0 && tp < 15){ tmp[tp++] = '0' + (code % 10); code /= 10; }
                while(tp--) buf[pos++] = tmp[tp];
            }
            buf[pos] = 0;
            print(buf);
            print("]\n");

            extern void process_exit_current(int exit_code);
            process_exit_current(code);
            return 0;
        }

        /* ── SYS_IOCTL_SCREEN ────────────────────────────────── */
        case SYS_IOCTL_SCREEN: {
            int cmd = (int)arg1;
            extern void clear_screen(void);
            extern void move_cursor(int x, int y);
            switch(cmd){
                case SCREEN_CLEAR:    clear_screen();                        return 0;
                case SCREEN_HOME:     move_cursor(0, 0);                     return 0;
                case SCREEN_SETCURSOR: move_cursor((int)arg3, (int)arg2);   return 0;
                default: return -1;
            }
        }

        /* ── SYS_GETCMDARGS (legacy flat string) ─────────────── */
        case 250: {
            char *buf     = (char *)arg1;
            uint64_t size = arg2;
            if(!buf || size == 0) return -1;
            if(current_process && current_process->exec_argc > 1){
                uint64_t out = 0;
                for(int i = 1; i < current_process->exec_argc && out < size - 1; i++){
                    const char *s = current_process->exec_argv_data
                                  + current_process->exec_argv_offsets[i];
                    if(i > 1 && out < size - 1) buf[out++] = ' ';
                    while(*s && out < size - 1) buf[out++] = *s++;
                }
                buf[out] = '\0';
                return (int32_t)out;
            }
            buf[0] = '\0';
            return 0;
        }

        /* ── SYS_GETARGC ─────────────────────────────────────── */
        case 260: {
            if(current_process) return current_process->exec_argc;
            return 0;
        }

        /* ── SYS_GETARGV ─────────────────────────────────────── */
        case 261: {
            int idx   = (int)arg1;
            char *buf = (char *)arg2;
            int bufsz = (int)arg3;
            if(!buf || bufsz <= 0) return -1;
            if(!current_process || idx < 0 || idx >= current_process->exec_argc){
                if(buf) buf[0] = '\0';
                return 0;
            }
            const char *p = current_process->exec_argv_data;
            for(int n = 0; n < idx; n++){ while(*p) p++; p++; }
            int len = 0;
            while(*p && len < bufsz - 1) buf[len++] = *p++;
            buf[len] = '\0';
            return len;
        }

        /* ── SYS_DRAW_PIXEL ──────────────────────────────────── */
        case SYS_DRAW_PIXEL: {
            int x = (int)arg1; int y = (int)arg2; uint32_t color = (uint32_t)arg3;
            if(x < 0 || x >= 1920 || y < 0 || y >= 1080) return -1;
            volatile uint32_t *fb = (volatile uint32_t *)0xE0000000;
            fb[y * 1920 + x] = color;
            return 0;
        }

        /* ── SYS_CLEAR_SCREEN ────────────────────────────────── */
        case SYS_CLEAR_SCREEN: {
            uint32_t color = (uint32_t)arg1;
            volatile uint32_t *fb = (volatile uint32_t *)0xE0000000;
            for(int y = 0; y < 1080; y++)
                for(int x = 0; x < 1920; x++)
                    fb[y * 1920 + x] = color;
            return 0;
        }

        /* ── SYS_FILL_RECT ───────────────────────────────────── */
        case SYS_FILL_RECT: {
            int x = (int)arg1, y = (int)arg2, w = (int)arg3, h = (int)arg4;
            uint32_t color = (uint32_t)arg5;
            volatile uint32_t *fb = (volatile uint32_t *)0xE0000000;
            for(int dy = 0; dy < h; dy++){
                int py = y + dy;
                if(py < 0 || py >= 1080) continue;
                for(int dx = 0; dx < w; dx++){
                    int px = x + dx;
                    if(px < 0 || px >= 1920) continue;
                    fb[py * 1920 + px] = color;
                }
            }
            return 0;
        }

        case SYS_LSUSB: {
    usb_lsusb_entry_t *out = (usb_lsusb_entry_t *)arg1;
    int max = (int)arg2;
    int count = 0;

    for (int i = 0; i < MAX_USB_DEVICES && count < max; i++) {
        if (!usb_devices[i].used || !usb_devices[i].connected) continue;
        out[count].addr       = usb_devices[i].addr;
        out[count].vendor_id  = usb_devices[i].vendor_id;
        out[count].product_id = usb_devices[i].product_id;
        out[count].class      = usb_devices[i].class;
        out[count].subclass   = usb_devices[i].subclass;
        out[count].port       = usb_devices[i].port;
        out[count].usb_bus        = usb_devices[i].usb_bus;
        count++;
    }
    return count;
}

        /* ── Networking ──────────────────────────────────────── */
        case SYS_NET_GETINFO: {
            net_info_t *info = (net_info_t *)arg1;
            if(!info) return -1;
            net_get_info(info->mac, &info->ip, &info->gateway, &info->netmask);
            return 0;
        }
        case SYS_NET_CONNECT: {
            return tcp_connect(arg1, (uint16_t)arg2, (uint16_t)arg3);
        }
        case SYS_NET_SEND: {
            return tcp_send((int)arg1, (uint8_t *)arg2, arg3);
        }
        case SYS_NET_RECV: {
            return tcp_recv((int)arg1, (uint8_t *)arg2, arg3);
        }
        case SYS_NET_CLOSE: {
            tcp_close((int)arg1);
            return 0;
        }
        case SYS_NET_CONNECTED: {
            return tcp_is_connected((int)arg1);
        }
        case SYS_NET_POLL: {
            net_poll();
            return 0;
        }
        case SYS_NET_ARP: {
            arp_request(arg1);
            return 0;
        }
        case SYS_NET_PING: {
            return icmp_ping((ip_addr_t)arg1);
        }
        case SYS_NET_PING_WAIT: {
            uint64_t loops = arg1;
            if(icmp_got_reply){ icmp_clear_reply_flag(); return 1; }
            while(loops--){
                net_poll();
                if(icmp_got_reply){ icmp_clear_reply_flag(); return 1; }
            }
            icmp_clear_pending_on_timeout();
            icmp_clear_reply_flag();
            return 0;
        }

        /* ── Unknown ─────────────────────────────────────────── */
        default: {
            print_color("[Unknown syscall: ", VGA_COLOR_LIGHT_RED);
            print_int_helper((int)syscall_num);
            print("]\n");
            return -1;
        }
    }
}
