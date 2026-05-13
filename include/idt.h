#ifndef IDT_H
#define IDT_H

#include <stdint.h>

void idt_init(void);
void idt_set_entry_pub(int i, uint32_t base, uint16_t sel, uint8_t flags);

#endif