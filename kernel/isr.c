#include "../include/vga.h"

/* Nombres de las 16 excepciones */
static const char *exception_names[] = {
    "Division by Zero",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved"
};

/* Estructura que refleja el stack en isr_common */
typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} __attribute__((packed)) registers_t;

void isr_handler(registers_t regs) {
    if (regs.int_no == 1 || regs.int_no == 6)   // debug y invalid opcode desactivados por ahora
        return;

    vga_puts_color("\n[EXCEPCION] ", VGA_LIGHT_RED, VGA_BLACK);
    if (regs.int_no < 16)
        vga_puts(exception_names[regs.int_no]);
    vga_puts("\nKernel detenido.\n");
    while (1) {}
}