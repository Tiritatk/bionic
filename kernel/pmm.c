#include "../include/pmm.h"
#include "../include/kprintf.h"

#define MAX_FRAMES (1024 * 1024)          /* soporta hasta 4 GB */
#define BITMAP_SIZE (MAX_FRAMES / 32)     /* uint32_t por cada 32 frames */

static uint32_t bitmap[BITMAP_SIZE];
static uint32_t total_frames = 0;
static uint32_t free_frames  = 0;

/* Símbolo del linker — dirección donde acaba el kernel */
extern uint32_t _kernel_end;

static inline void bitmap_set(uint32_t frame) {
    bitmap[frame / 32] |= (1 << (frame % 32));
}

static inline void bitmap_clear(uint32_t frame) {
    bitmap[frame / 32] &= ~(1 << (frame % 32));
}

static inline int bitmap_test(uint32_t frame) {
    return bitmap[frame / 32] & (1 << (frame % 32));
}

void pmm_init(multiboot_info_t *mb) {
    /* Marca todo como ocupado por defecto */
    for (int i = 0; i < BITMAP_SIZE; i++)
        bitmap[i] = 0xFFFFFFFF;

    /* Dirección donde acaba el kernel — no tocar por debajo */
    uint32_t kernel_end = (uint32_t)&_kernel_end;

    /* Recorre el mapa de memoria de Multiboot */
    mmap_entry_t *entry = (mmap_entry_t *)mb->mmap_addr;
    mmap_entry_t *end   = (mmap_entry_t *)(mb->mmap_addr + mb->mmap_length);

    while (entry < end) {
        /* Solo nos interesan las regiones de RAM disponible */
        if (entry->type == 1) {
            uint32_t addr  = (uint32_t)entry->addr;
            uint32_t len   = (uint32_t)entry->len;
            uint32_t frame = addr / PAGE_SIZE;
            uint32_t count = len  / PAGE_SIZE;

            for (uint32_t i = 0; i < count; i++) {
                uint32_t phys = (frame + i) * PAGE_SIZE;

                /* No liberar el primer MB ni la zona del kernel */
                if (phys < 0x100000 || phys < kernel_end) {
                    total_frames++;
                    continue;
                }

                if (frame + i < MAX_FRAMES) {
                    bitmap_clear(frame + i);
                    total_frames++;
                    free_frames++;
                }
            }
        }

        /* Avanza a la siguiente entrada (size no incluye el campo size) */
        entry = (mmap_entry_t *)((uint32_t)entry + entry->size + sizeof(uint32_t));
    }

    kprintf_color(VGA_LIGHT_GREEN, VGA_BLACK, "[OK] ");
    kprintf("PMM inicializado: %u MB libres / %u MB totales\n",
            (free_frames  * PAGE_SIZE) / (1024 * 1024),
            (total_frames * PAGE_SIZE) / (1024 * 1024));
}

uint32_t pmm_alloc(void) {
    if (free_frames == 0)
        return 0;   /* sin memoria */

    for (uint32_t i = 0; i < MAX_FRAMES / 32; i++) {
        if (bitmap[i] == 0xFFFFFFFF)
            continue;   /* todos ocupados en este bloque de 32 */

        for (uint32_t j = 0; j < 32; j++) {
            uint32_t frame = i * 32 + j;
            if (!bitmap_test(frame)) {
                bitmap_set(frame);
                free_frames--;
                return frame * PAGE_SIZE;
            }
        }
    }

    return 0;
}

void pmm_free(uint32_t addr) {
    uint32_t frame = addr / PAGE_SIZE;
    if (bitmap_test(frame)) {
        bitmap_clear(frame);
        free_frames++;
    }
}

uint32_t pmm_free_frames(void)  { return free_frames;  }
uint32_t pmm_total_frames(void) { return total_frames; }