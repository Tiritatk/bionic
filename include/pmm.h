#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include "multiboot.h"

#define PAGE_SIZE 4096

void     pmm_init(multiboot_info_t *mb);
uint32_t pmm_alloc(void);          /* devuelve dirección física del frame */
void     pmm_free(uint32_t addr);
uint32_t pmm_free_frames(void);
uint32_t pmm_total_frames(void);

#endif