#ifndef ATA_H
#define ATA_H

#include <stdint.h>

#define ATA_SECTOR_SIZE 512

void ata_init(void);

int ata_identify(void);

int ata_read_sector(uint32_t lba, uint8_t* buffer);
int ata_write_sector(uint32_t lba, const uint8_t* buffer);

uint8_t ata_is_present(void);
uint32_t ata_total_sectors(void);
uint32_t ata_size_mb(void);

#endif