#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>

typedef void (*irq_handler_t)(void);

void irq_init(void);
void irq_register(int irq, irq_handler_t handler);

#endif