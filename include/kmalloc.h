#ifndef KMALLOC_H
#define KMALLOC_H

#include <stdint.h>

void kmalloc_init(void);

void* kmalloc(uint32_t size);
void* kcalloc(uint32_t count, uint32_t size);
void kfree(void* ptr);

uint32_t kmalloc_used(void);
uint32_t kmalloc_total(void);

#endif