#include "../include/gdt.h"
#include "../include/idt.h"
#include "../include/irq.h"
#include "../include/keyboard.h"
#include "../include/kprintf.h"
#include "../include/vga.h"
#include "../include/shell.h"
#include "../include/pmm.h"
#include "../include/multiboot.h"
#include "../include/fs.h"
#include "../include/kmalloc.h"
#include "../include/gui.h"
#include "../include/mouse.h"
#include "../include/ata.h"
#include "../include/bionicfs.h"

void kernel_main(uint32_t mb_magic, multiboot_info_t *mb) {
    gdt_init();
    idt_init();
    irq_init();
    keyboard_init();
    mouse_init();
    vga_init();
    gui_init_from_multiboot(mb);
    pmm_init(mb);
    kmalloc_init();
    fs_init();
    ata_init();

    if (bionicfs_load() == 0) {
    kprintf("BIONICFS: filesystem cargado desde disco\n");
    } else {
    kprintf("BIONICFS: no hay filesystem guardado\n");
    }

    /* Mensajes con distintos colores */
    vga_puts_color("[OK] ", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("VGA Driver loaded.\n");

    vga_puts_color("[OK] ", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("Heap kmalloc initialized.\n");

    vga_puts_color("[OK] ", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("Terminal Abstraction loaded.\n");

    vga_puts_color("[OK] ", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("GDT loaded.\n");

    vga_puts_color("[OK] ", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("IDT loaded.\n");

    vga_puts_color("[OK] ", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("Keyboard Driver loaded.\n");

    vga_puts_color("[OK] ", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("RamFS loaded.\n");

    vga_puts_color("[INFO] ", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("Text mode: 80x25, 16 colors available\n");

    vga_puts("\n");

        /* Título con color */
    vga_puts_color("Welcome to Bionic Kernel 0.7\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("\n");

    // kprintf_color(VGA_YELLOW, VGA_BLACK, "\nEscribe algo:\n");

    keyboard_init();
    __asm__ volatile("sti");

    if (gui_has_framebuffer()) {
        gui_demo();
    } else {
        shell_init();
    }

    while (1) {}
}