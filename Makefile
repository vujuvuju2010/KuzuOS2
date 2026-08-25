CC       = gcc
AS       = nasm
LD       = ld

CFLAGS   = -m64 -mcmodel=large -fno-pie -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-red-zone -Ilibc/include -Isrc/
ASFLAGS  = -f elf64
LDFLAGS  = -m elf_x86_64 -T linker.ld

# Top-level target
all: kuzuos.iso

# --------------------------------------------------------------------
# Kernel binary
# --------------------------------------------------------------------
kernel.bin: boot.o kernel.o usb.o usbmsc.o keyboardusb.o vfs.o tty.o memory.o pmm.o vmm.o interrupts.o isr.o keyboard.o \
            irq.o irq_asm.o process.o filesystem.o shell.o vga.o \
            syscall.o syscall_router.o fatfs_ff.o fatfs_diskio.o banner.o gdt.o xchi.o \
            gdt_flush.o loader_kernel.o enter_user_mode.o \
            context_switch.o elf_loader.o \
            z_printf.o z_utils.o z_err.o z_trampo.o \
            net.o e1000.o ethernet.o arp.o net_ip.o tcp.o \
            keymap_loader.o
	$(LD) $(LDFLAGS) -o $@ $^

# --------------------------------------------------------------------
# Assembly files
# --------------------------------------------------------------------
boot.o: src/boot.asm
	$(AS) $(ASFLAGS) -o $@ $<

gdt_flush.o: src/gdt_flush.asm
	$(AS) $(ASFLAGS) -o $@ $<

isr.o: src/isr.asm
	$(AS) $(ASFLAGS) -o $@ $<

irq_asm.o: src/irq.asm
	$(AS) $(ASFLAGS) -o $@ $<

enter_user_mode.o: src/enter_user_mode.asm
	$(AS) $(ASFLAGS) -o $@ $<

context_switch.o: src/context_switch.asm
	$(AS) $(ASFLAGS) -o $@ $<

z_trampo.o: src/z_trampo.S
	$(AS) $(ASFLAGS) -o $@ $<

z_syscall.o: src/z_syscall.S
	$(AS) $(ASFLAGS) -o $@ $<

# --------------------------------------------------------------------
# C files (kernel)
# --------------------------------------------------------------------
usb.o: src/usb.c 
	$(CC) $(CFLAGS) -c -o $@ $<
usbmsc.o: src/usbmsc.c 
	$(CC) $(CFLAGS) -c -o $@ $<

xchi.o: src/xchi.c
	$(CC) $(CFLAGS) -c -o $@ $<

vfs.o: src/kuzulib/fs/vfs.c 
	$(CC) $(CFLAGS) -c -o $@ $<

keyboardusb.o: src/keyboardusb.c 
	$(CC) $(CFLAGS) -c -o $@ $<

tty.o: src/tty.c 
	$(CC) $(CFLAGS) -c -o $@ $<

kernel.o: src/kernel.c
	$(CC) $(CFLAGS) -c -o $@ $<

gdt.o: src/gdt.c
	$(CC) $(CFLAGS) -c -o $@ $<

memory.o: src/memory.c
	$(CC) $(CFLAGS) -c -o $@ $<

pmm.o: src/pmm.c
	$(CC) $(CFLAGS) -c -o $@ $<

vmm.o: src/vmm.c
	$(CC) $(CFLAGS) -c -o $@ $<

interrupts.o: src/interrupts.c
	$(CC) $(CFLAGS) -c -o $@ $<

keyboard.o: src/keyboard.c
	$(CC) $(CFLAGS) -c -o $@ $<

keymap_loader.o: src/keymap_loader.c
	$(CC) $(CFLAGS) -c -o $@ $<

irq.o: src/irq.c
	$(CC) $(CFLAGS) -c -o $@ $<

process.o: src/process.c
	$(CC) $(CFLAGS) -c -o $@ $<

filesystem.o: src/filesystem.c
	$(CC) $(CFLAGS) -c -o $@ $<

shell.o: src/shell.c
	$(CC) $(CFLAGS) -c -o $@ $<

vga.o: src/vga.c
	$(CC) $(CFLAGS) -c -o $@ $<

syscall.o: src/syscall.c
	$(CC) $(CFLAGS) -c -o $@ $<

syscall_router.o: src/syscall_router.c
	$(CC) $(CFLAGS) -c -o $@ $<

fatfs_ff.o: src/fatfs/ff.c
	$(CC) $(CFLAGS) -c -o $@ $<

fatfs_diskio.o: src/fatfs/diskio.c
	$(CC) $(CFLAGS) -c -o $@ $<

banner.o: src/banner.c
	$(CC) $(CFLAGS) -c -o $@ $<

loader_kernel.o: src/loader_kernel.c
	$(CC) $(CFLAGS) -c -o $@ $<

elf_loader.o: src/elf_loader.c
	$(CC) $(CFLAGS) -c -o $@ $<

z_printf.o: src/kuzulib/stdio/z_printf.c
	$(CC) $(CFLAGS) -Isrc/ -c -o $@ $<

z_utils.o: src/kuzulib/string/z_utils.c
	$(CC) $(CFLAGS) -Isrc/ -c -o $@ $<

z_err.o: src/kuzulib/stdio/z_err.c
	$(CC) $(CFLAGS) -Isrc/ -c -o $@ $<


# --------------------------------------------------------------------
# Network stack
# --------------------------------------------------------------------
net.o: src/internet/net.c src/internet/net.h src/internet/ethernet.h src/internet/e1000.h
	$(CC) $(CFLAGS) -c -o $@ $<

e1000.o: src/internet/e1000.c src/internet/e1000.h src/internet/net.h
	$(CC) $(CFLAGS) -c -o $@ $<

ethernet.o: src/internet/ethernet.c src/internet/ethernet.h src/internet/arp.h src/internet/ip.h
	$(CC) $(CFLAGS) -c -o $@ $<

arp.o: src/internet/arp.c src/internet/arp.h src/internet/ethernet.h
	$(CC) $(CFLAGS) -c -o $@ $<

net_ip.o: src/internet/ip.c src/internet/ip.h src/internet/ethernet.h src/internet/arp.h src/internet/tcp.h
	$(CC) $(CFLAGS) -c -o $@ $<

tcp.o: src/internet/tcp.c src/internet/tcp.h src/internet/ip.h
	$(CC) $(CFLAGS) -c -o $@ $<

# --------------------------------------------------------------------
# kuzulib - ring 3 standard library
# --------------------------------------------------------------------
KUZULIB_CFLAGS = $(CFLAGS) -Isrc/

kuzulib:
	mkdir -p kuzulib

kuzulib/z_syscalls.o: src/kuzulib/syscall/z_syscalls.c | kuzulib
	$(CC) $(KUZULIB_CFLAGS) -c -o $@ $<

kuzulib/z_syscall.o: src/kuzulib/syscall/z_syscall.S | kuzulib
	$(AS) $(ASFLAGS) -o $@ $<

kuzulib/z_printf.o: src/kuzulib/stdio/z_printf.c | kuzulib
	$(CC) $(KUZULIB_CFLAGS) -c -o $@ $<

kuzulib/z_err.o: src/kuzulib/stdio/z_err.c | kuzulib
	$(CC) $(KUZULIB_CFLAGS) -c -o $@ $<

kuzulib/z_utils.o: src/kuzulib/string/z_utils.c | kuzulib
	$(CC) $(KUZULIB_CFLAGS) -c -o $@ $<

kuzulib/malloc.o: src/kuzulib/stdlib/malloc.c | kuzulib
	$(CC) $(KUZULIB_CFLAGS) -c -o $@ $<

kuzulib/setjmp.o: src/kuzulib/stdlib/setjmp.S | kuzulib
	$(AS) $(ASFLAGS) -o $@ $<

kuzulib/libc_syscall_wrappers.o: src/kuzulib/syscall/libc_syscall_wrappers.c | kuzulib
	$(CC) $(KUZULIB_CFLAGS) -c -o $@ $<

kuzulib/libc_vim_stubs.o: src/kuzulib/compat/libc_vim_stubs.c | kuzulib
	$(CC) $(KUZULIB_CFLAGS) -c -o $@ $<

libkuzu.a: kuzulib/z_printf.o kuzulib/z_syscalls.o kuzulib/z_utils.o kuzulib/z_err.o \
           kuzulib/malloc.o kuzulib/libc_vim_stubs.o \
           kuzulib/z_syscall.o kuzulib/setjmp.o
	ar rcs libkuzu.a $^

# --------------------------------------------------------------------
# User programs
# --------------------------------------------------------------------
echo: echo.o libkuzu.a
	$(LD) -m elf_x86_64 -s -Ttext=0x00400000 -o $@ echo.o libkuzu.a

echo.o: src/echo.c
	$(CC) $(CFLAGS) -c -o $@ $<

calc: calc.o libkuzu.a
	$(LD) -m elf_x86_64 -s -Ttext=0x00400000 -o $@ calc.o libkuzu.a

calc.o: src/calc.c
	$(CC) $(CFLAGS) -c -o $@ $<

tmux: tmux.o libkuzu.a
	$(LD) -m elf_x86_64 -s -Ttext=0x00400000 -o $@ tmux.o libkuzu.a

tmux.o: src/tmux.c
	$(CC) $(CFLAGS) -c -o $@ $<

ls: ls.o libkuzu.a
	$(LD) -m elf_x86_64 -s -Ttext=0x00400000 -o $@ ls.o libkuzu.a

ls.o: src/ls.c
	$(CC) $(CFLAGS) -c -o $@ $<

mkdir: mkdir.o libkuzu.a
	$(LD) -m elf_x86_64 -s -Ttext=0x00400000 -o $@ mkdir.o libkuzu.a

mkdir.o: src/mkdir.c
	$(CC) $(CFLAGS) -c -o $@ $<

clear: clear.o libkuzu.a
	$(LD) -m elf_x86_64 -s -Ttext=0x00400000 -o $@ clear.o libkuzu.a

clear.o: src/clear.c
	$(CC) $(CFLAGS) -c -o $@ $<

pwd: pwd.o libkuzu.a
	$(LD) -m elf_x86_64 -s -Ttext=0x00400000 -o $@ pwd.o libkuzu.a

pwd.o: src/pwd.c
	$(CC) $(CFLAGS) -c -o $@ $<

cd: cd.o libkuzu.a
	$(LD) -m elf_x86_64 -s -Ttext=0x00400000 -o $@ cd.o libkuzu.a

cd.o: src/cd.c
	$(CC) $(CFLAGS) -c -o $@ $<

cat: cat.o libkuzu.a
	$(LD) -m elf_x86_64 -s -Ttext=0x00400000 -o $@ cat.o libkuzu.a

cat.o: src/cat.c
	$(CC) $(CFLAGS) -c -o $@ $<

touch: touch.o libkuzu.a
	$(LD) -m elf_x86_64 -s -Ttext=0x00400000 -o $@ touch.o libkuzu.a

touch.o: src/touch.c
	$(CC) $(CFLAGS) -c -o $@ $<

whoami: whoami.o libkuzu.a
	$(LD) -m elf_x86_64 -s -Ttext=0x00400000 -o $@ whoami.o libkuzu.a

whoami.o: src/whoami.c
	$(CC) $(CFLAGS) -c -o $@ $<

date: date.o libkuzu.a
	$(LD) -m elf_x86_64 -s -Ttext=0x00400000 -o $@ date.o libkuzu.a

date.o: src/date.c
	$(CC) $(CFLAGS) -c -o $@ $<

uname: uname.o libkuzu.a
	$(LD) -m elf_x86_64 -s -Ttext=0x00400000 -o $@ uname.o libkuzu.a

uname.o: src/uname.c
	$(CC) $(CFLAGS) -c -o $@ $<

ip: ip_user.o
	$(LD) -m elf_x86_64 -s -Ttext=0x00400000 -e _start -o $@ $^

ip_user.o: src/ip.c
	$(CC) $(CFLAGS) -fno-stack-protector -c -o $@ $<

ping: ping.o
	$(LD) -m elf_x86_64 -s -Ttext=0x00400000 -o $@ $^

ping.o: src/ping.c
	$(CC) $(CFLAGS) -c -o $@ $<

vim: vim.o libkuzu.a
	$(LD) -m elf_x86_64 -s -Ttext=0x00400000 -o $@ vim.o libkuzu.a

vim.o: src/vim.c
	$(CC) $(CFLAGS) -c -o $@ $<

gif: gif.o libkuzu.a
	$(LD) -m elf_x86_64 -s -Ttext=0x00400000 -o $@ gif.o libkuzu.a

gif.o: src/gif.c
	$(CC) $(CFLAGS) -c -o $@ $<

lsusb: lsusb.o libkuzu.a
	$(LD) -m elf_x86_64 -s -Ttext=0x00400000 -o $@ lsusb.o libkuzu.a
lsusb.o: src/lsusb.c
	$(CC) $(CFLAGS) -c -o $@ $<

hlt: hlt.o libkuzu.a
	$(LD) -m elf_x86_64 -s -Ttext=0x00400000 -o $@ hlt.o libkuzu.a

hlt.o: src/hlt.c
	$(CC) $(CFLAGS) -c -o $@ $<

loadkeys: loadkeys.o libkuzu.a
	$(LD) -m elf_x86_64 -s -Ttext=0x00400000 -e _start -o $@ loadkeys.o libkuzu.a

loadkeys.o: src/loadkeys.c
	$(CC) $(CFLAGS) -c -o $@ $<

# TinyCC Compiler (using real TinyCC libtcc only, skip tcc.c main)
# All TinyCC modules are #included in libtcc.c when ONE_SOURCE=1
tcc: tcc_start.o tinycc_kuzuos.o tinycc_libtcc.o libkuzu.a
	$(LD) -m elf_x86_64 -s -Ttext=0x00400000 -o $@ tcc_start.o tinycc_kuzuos.o tinycc_libtcc.o libkuzu.a

tcc_start.o: src/tcc_start.c
	$(CC) $(CFLAGS) -c -o $@ $<

tinycc_kuzuos.o: src/tinycc_kuzuos.c
	$(CC) $(CFLAGS) -I./src/tinycc -c -o $@ $<

# Build TinyCC library (all modules are #included in libtcc.c)
tinycc_libtcc.o: src/tinycc/libtcc.c
	$(CC) $(CFLAGS) -I./src/tinycc -c -o $@ $<

libc_ctype.o:
	echo "int isalpha(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }" > /tmp/ctype_stub.c
	echo "int isdigit(int c) { return c >= '0' && c <= '9'; }" >> /tmp/ctype_stub.c
	echo "int isalnum(int c) { return isalpha(c) || isdigit(c); }" >> /tmp/ctype_stub.c
	$(CC) $(CFLAGS) -c -o $@ /tmp/ctype_stub.c

# --------------------------------------------------------------------
# Minimal C runtime stubs for TCC
# --------------------------------------------------------------------
lib/crt1.o: lib/crt1.S
	$(AS) $(ASFLAGS) -o $@ $<

lib/crti.o: lib/crti.S
	$(AS) $(ASFLAGS) -o $@ $<

lib/crtn.o: lib/crtn.S
	$(AS) $(ASFLAGS) -o $@ $<

# --------------------------------------------------------------------
# ISO
# --------------------------------------------------------------------
iso/boot/grub/grub.cfg:
	mkdir -p iso/boot/grub
	echo "set timeout=5" > $@
	echo "set default=0" >> $@
	echo "" >> $@
	echo "menuentry \"KuzuOS\" {" >> $@
	echo "    multiboot2 /boot/kernel.bin" >> $@
	echo "    boot" >> $@
	echo "}" >> $@

KBD_INC ?= /usr/share/kbd/keymaps/i386/include
KBD_COMPOSE ?= /usr/share/kbd/keymaps/include

kuzuos.iso: kernel.bin iso/boot/grub/grub.cfg echo calc tmux hlt ls mkdir clear pwd cd cat touch whoami date uname vim gif lsusb tcc ip ping loadkeys hello.c banner_frames/*.bin lib/crt1.o lib/crti.o lib/crtn.o keymaps/us.map keymaps/trq.map
	mkdir -p iso/boot iso/dev iso/lib iso/dev/keys
	cp kernel.bin iso/boot/
	cp echo calc tmux hlt ls mkdir clear pwd cd cat touch whoami date uname vim gif lsusb tcc ip ping loadkeys iso/dev/
	cp hello.c iso/dev/
	cp banner_frames/*.bin iso/dev/
	cp keymaps/*.map iso/dev/keys/
	cp keymaps/qwerty-layout iso/dev/keys/ 2>/dev/null || cp $(KBD_INC)/qwerty-layout.inc iso/dev/keys/qwerty-layout
	cp $(KBD_INC)/linux-with-alt-and-altgr.inc iso/dev/keys/linux-with-alt-and-altgr
	cp $(KBD_INC)/linux-keys-bare.inc iso/dev/keys/linux-keys-bare
	zcat $(KBD_INC)/euro1.map.gz > iso/dev/keys/euro1.map
	cp $(KBD_COMPOSE)/compose.latin1 iso/dev/keys/compose.latin1
	mv iso/dev/clear iso/dev/clr
	cp lib/crt1.o lib/crti.o lib/crtn.o iso/lib/
	grub-mkrescue -o $@ iso -J -R

# --------------------------------------------------------------------
# Utilities
# --------------------------------------------------------------------
clean:
	rm -f *.o kernel.bin kuzuos.iso echo calc tmux hlt ls mkdir clear pwd cd cat touch whoami date uname vim gif lsusb tcc ip ping loadkeys net_ip.o ip_user.o
	rm -f libkuzu.a kuzulib/*.o
	rm -f /tmp/ctype_stub.c libc_*.o
	rm -rf iso



.PHONY: all clean test kuzulib

