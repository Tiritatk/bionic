#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>

int fat32_mount(uint8_t drive);
uint8_t fat32_is_mounted(void);

uint8_t fat32_drive(void);
uint32_t fat32_total_sectors(void);
uint32_t fat32_fat_start_lba(void);
uint32_t fat32_data_start_lba(void);
uint32_t fat32_root_cluster(void);
uint32_t fat32_sectors_per_cluster(void);
uint32_t fat32_bytes_per_sector(void);
uint32_t fat32_reserved_sectors(void);
uint32_t fat32_num_fats(void);
uint32_t fat32_fat_size(void);

int fat32_list_root(void (*print_func)(const char*));
int fat32_read_root_file(const char* filename, void (*print_func)(const char*));
int fat32_import_root_file(const char* fat_filename, const char* ramfs_path);

#endif