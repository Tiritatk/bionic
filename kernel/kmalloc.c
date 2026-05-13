#include "../include/kmalloc.h"
#include "../include/pmm.h"
#include "../include/kprintf.h"

#define KMALLOC_PAGE_SIZE 4096
#define KMALLOC_MAGIC     0xC0FFEE42

typedef struct heap_block {
    uint32_t magic;
    uint32_t size;
    uint32_t free;
    struct heap_block* next;
} heap_block_t;

static heap_block_t* heap_head = 0;
static uint32_t heap_used_bytes = 0;
static uint32_t heap_total_bytes = 0;

static uint32_t align_up(uint32_t value, uint32_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

static void heap_zero(void* ptr, uint32_t size) {
    uint8_t* bytes = (uint8_t*)ptr;

    for (uint32_t i = 0; i < size; i++) {
        bytes[i] = 0;
    }
}

void kmalloc_init(void) {
    heap_head = 0;
    heap_used_bytes = 0;
    heap_total_bytes = 0;

//    kprintf("kmalloc iniciado\n");
}

static heap_block_t* request_page(void) {
    heap_block_t* block = (heap_block_t*)pmm_alloc();

    if (!block) {
        kprintf("kmalloc: sin memoria fisica\n");
        return 0;
    }

    block->magic = KMALLOC_MAGIC;
    block->size = KMALLOC_PAGE_SIZE - sizeof(heap_block_t);
    block->free = 1;
    block->next = 0;

    heap_total_bytes += KMALLOC_PAGE_SIZE;

    if (!heap_head) {
        heap_head = block;
    } else {
        heap_block_t* current = heap_head;

        while (current->next) {
            current = current->next;
        }

        current->next = block;
    }

    return block;
}

static void split_block(heap_block_t* block, uint32_t size) {
    /*
       Solo partimos el bloque si sobra espacio suficiente para otro bloque util.
    */
    if (block->size <= size + sizeof(heap_block_t) + 8) {
        return;
    }

    heap_block_t* new_block = (heap_block_t*)((uint8_t*)block + sizeof(heap_block_t) + size);

    new_block->magic = KMALLOC_MAGIC;
    new_block->size = block->size - size - sizeof(heap_block_t);
    new_block->free = 1;
    new_block->next = block->next;

    block->size = size;
    block->next = new_block;
}

static void merge_free_blocks(void) {
    heap_block_t* current = heap_head;

    while (current && current->next) {
        uint8_t* current_end = (uint8_t*)current + sizeof(heap_block_t) + current->size;

        /*
           Solo fusionamos si los bloques estan fisicamente juntos.
           Como las paginas del PMM podrian no ser contiguas, hay que comprobarlo.
        */
        if (current->free && current->next->free && current_end == (uint8_t*)current->next) {
            current->size += sizeof(heap_block_t) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

void* kmalloc(uint32_t size) {
    if (size == 0) {
        return 0;
    }

    size = align_up(size, 8);

    if (size > KMALLOC_PAGE_SIZE - sizeof(heap_block_t)) {
        kprintf("kmalloc: bloque demasiado grande: %u bytes\n", size);
        return 0;
    }

    heap_block_t* current = heap_head;

    while (current) {
        if (current->magic != KMALLOC_MAGIC) {
            kprintf("kmalloc: heap corrupto\n");
            return 0;
        }

        if (current->free && current->size >= size) {
            split_block(current, size);

            current->free = 0;
            heap_used_bytes += current->size;

            return (void*)((uint8_t*)current + sizeof(heap_block_t));
        }

        current = current->next;
    }

    current = request_page();

    if (!current) {
        return 0;
    }

    split_block(current, size);

    current->free = 0;
    heap_used_bytes += current->size;

    return (void*)((uint8_t*)current + sizeof(heap_block_t));
}

void* kcalloc(uint32_t count, uint32_t size) {
    uint32_t total = count * size;

    void* ptr = kmalloc(total);

    if (!ptr) {
        return 0;
    }

    heap_zero(ptr, total);

    return ptr;
}

void kfree(void* ptr) {
    if (!ptr) {
        return;
    }

    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));

    if (block->magic != KMALLOC_MAGIC) {
        kprintf("kfree: puntero invalido\n");
        return;
    }

    if (block->free) {
        kprintf("kfree: doble liberacion detectada\n");
        return;
    }

    block->free = 1;

    if (heap_used_bytes >= block->size) {
        heap_used_bytes -= block->size;
    } else {
        heap_used_bytes = 0;
    }

    merge_free_blocks();
}

uint32_t kmalloc_used(void) {
    return heap_used_bytes;
}

uint32_t kmalloc_total(void) {
    return heap_total_bytes;
}