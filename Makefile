CC = gcc
AS = nasm
LD = ld

CFLAGS = -m32 -fno-pie -nostdlib -nostdinc -fno-builtin -fno-stack-protector
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld

# Hedefler
all: kuzuos.iso

# Kernel binary oluştur
kernel.bin: boot.o kernel.o memory.o interrupts.o isr.o keyboard.o irq.o irq_asm.o process.o filesystem.o shell.o vga.o loader_kernel.o loader.o z_utils.o z_printf.o z_err.o z_syscall.o z_trampo.o syscall.o fatfs_ff.o fatfs_diskio.o banner.o exit_handler.o gdt.o gdt_flush.o
	$(LD) $(LDFLAGS) -o $@ $^

# Assembly dosyalarını derle
boot.o: src/boot.asm
	$(AS) $(ASFLAGS) -o $@ $<
gdt_flush.o: src/gdt_flush.asm
	$(AS) $(ASFLAGS) -o $@ $<

isr.o: src/isr.asm
	$(AS) $(ASFLAGS) -o $@ $<

irq_asm.o: src/irq.asm
	$(AS) $(ASFLAGS) -o $@ $<

exit_handler.o: src/exit_handler.asm
	$(AS) $(ASFLAGS) -o $@ $<

z_trampo.o: src/z_trampo.S
	$(AS) $(ASFLAGS) -o $@ $<

z_syscall.o: src/z_syscall.S
	$(AS) $(ASFLAGS) -o $@ $<

# C dosyalarını derle
kernel.o: src/kernel.c
	$(CC) $(CFLAGS) -c -o $@ $<

gdt.o: src/gdt.c
	$(CC) $(CFLAGS) -c -o $@ $<


memory.o: src/memory.c
	$(CC) $(CFLAGS) -c -o $@ $<

interrupts.o: src/interrupts.c
	$(CC) $(CFLAGS) -c -o $@ $<

keyboard.o: src/keyboard.c
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

loader_kernel.o: src/loader_kernel.c
	$(CC) $(CFLAGS) -c -o $@ $<

loader.o: src/loader.c
	$(CC) $(CFLAGS) -c -o $@ $<

z_utils.o: src/z_utils.c
	$(CC) $(CFLAGS) -c -o $@ $<

z_printf.o: src/z_printf.c
	$(CC) $(CFLAGS) -c -o $@ $<

z_err.o: src/z_err.c
	$(CC) $(CFLAGS) -c -o $@ $<

# z_syscalls.o not needed - functions are in loader_kernel.c
# z_syscalls.o: src/z_syscalls.c
#	$(CC) $(CFLAGS) -DKERNEL_SPACE -c -o $@ $<

syscall.o: src/syscall.c src/syscall.h
	$(CC) $(CFLAGS) -c -o $@ $<

fatfs_ff.o: src/fatfs/ff.c src/fatfs/ff.h src/fatfs/ffconf.h src/fatfs/integer.h src/fatfs/diskio.h
	$(CC) $(CFLAGS) -c -o $@ $<

fatfs_diskio.o: src/fatfs/diskio.c src/fatfs/diskio.h
	$(CC) $(CFLAGS) -c -o $@ $<

banner.o: src/banner.c src/banner.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Hello World assembly program
hello.o: src/hello.asm
	$(AS) $(ASFLAGS) -o $@ $<

hello: hello.o
	$(LD) -m elf_i386 -s -Ttext=0x00400000 -o $@ $<

# ISO dizinini hazırla
iso/boot/grub/grub.cfg:
	mkdir -p iso/boot/grub
	echo "set timeout=5" > $@
	echo "set default=0" >> $@
	echo "" >> $@
	echo "menuentry \"KuzuOS\" {" >> $@
	echo "    multiboot2 /boot/kernel.bin" >> $@
	echo "    boot" >> $@
	echo "}" >> $@

# ISO oluştur
kuzuos.iso: kernel.bin iso/boot/grub/grub.cfg hello
	mkdir -p iso/boot
	cp kernel.bin iso/boot/
	cp hello iso/hello
	# Embed banner frames if they exist
	if [ -d banner_frames ]; then \
		mkdir -p iso/banner_frames; \
		cp banner_frames/*.bin iso/banner_frames/ 2>/dev/null || true; \
		cp banner_frames/*.bin iso/ 2>/dev/null || true; \
	fi
	grub-mkrescue -o $@ iso

# Temizle
clean:
	rm -f *.o kernel.bin kuzuos.iso hello
	rm -rf iso

# Test et (QEMU ile)
test: kuzuos.iso
	qemu-system-i386 -cdrom kuzuos.iso

.PHONY: all clean test 
