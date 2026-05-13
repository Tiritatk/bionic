#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

#define MULTIBOOT_MAGIC 0x2BADB002

/* Multiboot info flags */
#define MULTIBOOT_INFO_MEMORY      (1 << 0)
#define MULTIBOOT_INFO_BOOTDEV     (1 << 1)
#define MULTIBOOT_INFO_CMDLINE     (1 << 2)
#define MULTIBOOT_INFO_MODS        (1 << 3)
#define MULTIBOOT_INFO_AOUT_SYMS   (1 << 4)
#define MULTIBOOT_INFO_ELF_SHDR    (1 << 5)
#define MULTIBOOT_INFO_MEM_MAP     (1 << 6)
#define MULTIBOOT_INFO_DRIVE_INFO  (1 << 7)
#define MULTIBOOT_INFO_CONFIG_TBL  (1 << 8)
#define MULTIBOOT_INFO_BOOT_LOADER (1 << 9)
#define MULTIBOOT_INFO_APM_TABLE   (1 << 10)
#define MULTIBOOT_INFO_VBE_INFO    (1 << 11)
#define MULTIBOOT_INFO_FRAMEBUFFER (1 << 12)

/* Entrada del mapa de memoria */
typedef struct {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;    /* 1 = RAM disponible, otros = reservado */
} __attribute__((packed)) mmap_entry_t;

/* Estructura principal que rellena GRUB / Multiboot v1 */
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;      /* KB de RAM baja (< 1MB) */
    uint32_t mem_upper;      /* KB de RAM alta (> 1MB) */
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;    /* tamaño total del mmap en bytes */
    uint32_t mmap_addr;      /* dirección física del mmap */

    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;

    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;

    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
    uint16_t framebuffer_reserved;
} __attribute__((packed)) multiboot_info_t;

#endif
