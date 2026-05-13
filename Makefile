CC      = gcc
AS      = nasm
CFLAGS  = -m32 -march=i686 -mtune=i686 \
          -ffreestanding -fno-builtin -fno-stack-protector \
          -fno-pic -fno-asynchronous-unwind-tables \
          -mno-sse -mno-sse2 -mno-sse3 -mno-mmx \
          -mno-80387 \
          -O0 -Wall -Wextra -Iinclude
LDFLAGS = -m elf_i386 -T linker.ld --nostdlib

OBJS = boot.o boot/gdt.o boot/idt.o boot/isr.o boot/irq.o \
       kernel/kernel.o kernel/vga.o kernel/gdt.o \
       kernel/idt.o kernel/isr.o kernel/irq.o \
       kernel/keyboard.o kernel/mouse.o kernel/kprintf.o kernel/shell.o \
       kernel/pmm.o kernel/kmalloc.o kernel/fs.o kernel/gui.o kernel/input.o

all: myos.iso

boot.o: boot/boot.asm
	$(AS) -f elf32 boot/boot.asm -o boot.o

boot/%.o: boot/%.asm
	$(AS) -f elf32 $< -o $@

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel.bin: $(OBJS)
	ld $(LDFLAGS) -o kernel.bin $(OBJS)

myos.iso: kernel.bin
	mkdir -p isodir/boot/grub
	cp kernel.bin isodir/boot/kernel.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o myos.iso isodir

run: myos.iso
	qemu-system-i386 -cdrom myos.iso

clean:
	rm -f *.o kernel/*.o kernel.bin myos.iso
	rm -rf isodir