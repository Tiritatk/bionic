#include "../include/irq.h"
#include "../include/idt.h"
#include "../include/io.h"

/* Puertos del PIC maestro y esclavo */
#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI   0x20   /* End Of Interrupt */

/* Tabla de handlers registrados, uno por IRQ */
static irq_handler_t irq_handlers[16] = {0};

/* IRQs declaradas en irq.asm */
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

static inline void io_wait(void) {
    outb(0x80, 0);   /* puerto 0x80 = dummy, solo introduce un delay */
}

static void pic_remap(void) {
    /* Guarda las máscaras actuales */
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    /* Inicialización en cascada (ICW1) */
    outb(PIC1_CMD, 0x11); io_wait();
    outb(PIC2_CMD, 0x11); io_wait();

    /* Vector base: IRQ0-7 → INT 32-39, IRQ8-15 → INT 40-47 (ICW2) */
    outb(PIC1_DATA, 0x20); io_wait();
    outb(PIC2_DATA, 0x28); io_wait();

    /* Cascada: PIC1 tiene esclavo en IRQ2 (ICW3) */
    outb(PIC1_DATA, 0x04); io_wait();
    outb(PIC2_DATA, 0x02); io_wait();

    /* Modo 8086 (ICW4) */
    outb(PIC1_DATA, 0x01); io_wait();
    outb(PIC2_DATA, 0x01); io_wait();

    /* Restaura máscaras */
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void irq_init(void) {
    pic_remap();

    /* Registra las 16 entradas en la IDT a partir de la 32 */
    extern void idt_set_entry_pub(int, uint32_t, uint16_t, uint8_t);
    idt_set_entry_pub(32, (uint32_t)irq0,  0x08, 0x8E);
    idt_set_entry_pub(33, (uint32_t)irq1,  0x08, 0x8E);
    idt_set_entry_pub(34, (uint32_t)irq2,  0x08, 0x8E);
    idt_set_entry_pub(35, (uint32_t)irq3,  0x08, 0x8E);
    idt_set_entry_pub(36, (uint32_t)irq4,  0x08, 0x8E);
    idt_set_entry_pub(37, (uint32_t)irq5,  0x08, 0x8E);
    idt_set_entry_pub(38, (uint32_t)irq6,  0x08, 0x8E);
    idt_set_entry_pub(39, (uint32_t)irq7,  0x08, 0x8E);
    idt_set_entry_pub(40, (uint32_t)irq8,  0x08, 0x8E);
    idt_set_entry_pub(41, (uint32_t)irq9,  0x08, 0x8E);
    idt_set_entry_pub(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set_entry_pub(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_entry_pub(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set_entry_pub(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set_entry_pub(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set_entry_pub(47, (uint32_t)irq15, 0x08, 0x8E);
}

void irq_register(int irq, irq_handler_t handler) {
    if (irq >= 0 && irq < 16)
        irq_handlers[irq] = handler;
}

/* Llamado desde irq.asm — despacha al handler registrado */
void irq_handler(int irq) {
    /* Manda EOI al PIC para que pueda enviar más interrupciones */
    if (irq >= 8)
        outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);

    if (irq_handlers[irq])
        irq_handlers[irq]();
}