#ifndef ATA_H
#define ATA_H

#include <stdint.h>

#define ATA_SECTOR_SIZE 512
#define ATA_DRIVE_MASTER 0
#define ATA_DRIVE_SLAVE  1

void ata_init(void);

int ata_identify(void);

int ata_read_sector(uint32_t lba, uint8_t* buffer);
int ata_write_sector(uint32_t lba, const uint8_t* buffer);

uint8_t ata_is_present(void);
uint32_t ata_total_sectors(void);
uint32_t ata_size_mb(void);

uint8_t ata_is_present_drive(uint8_t drive);
uint32_t ata_total_sectors_drive(uint8_t drive);
uint32_t ata_size_mb_drive(uint8_t drive);

int ata_identify_drive(uint8_t drive);
int ata_read_sector_drive(uint8_t drive, uint32_t lba, uint8_t* buffer);
int ata_write_sector_drive(uint8_t drive, uint32_t lba, const uint8_t* buffer);

#endif