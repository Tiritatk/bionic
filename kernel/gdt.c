#include "../include/gdt.h"

/* Cada entrada de la GDT son 8 bytes */
typedef struct {
    uint16_t limit_low;    /* bits 0-15 del límite */
    uint16_t base_low;     /* bits 0-15 de la base */
    uint8_t  base_mid;     /* bits 16-23 de la base */
    uint8_t  access;       /* tipo, privilegio, presente */
    uint8_t  granularity;  /* límite alto + flags */
    uint8_t  base_high;    /* bits 24-31 de la base */
} __attribute__((packed)) gdt_entry_t;

/* El puntero que carga lgdt */
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

#define GDT_ENTRIES 3
static gdt_entry_t gdt[GDT_ENTRIES];
static gdt_ptr_t   gdt_ptr;

/* Declarada en gdt.asm */
extern void gdt_flush(uint32_t);

static void gdt_set_entry(int i,
                          uint32_t base,
                          uint32_t limit,
                          uint8_t  access,
                          uint8_t  gran)
{
    gdt[i].base_low   = base & 0xFFFF;
    gdt[i].base_mid   = (base >> 16) & 0xFF;
    gdt[i].base_high  = (base >> 24) & 0xFF;
    gdt[i].limit_low  = limit & 0xFFFF;
    gdt[i].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[i].access     = access;
}

void gdt_init(void) {
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base  = (uint32_t)&gdt;

    gdt_set_entry(0, 0, 0,          0x00, 0x00); /* null descriptor */
    gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); /* código ring 0  */
    gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF); /* datos ring 0   */

    gdt_flush((uint32_t)&gdt_ptr);
}