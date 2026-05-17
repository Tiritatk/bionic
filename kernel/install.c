#include "../include/install.h"
#include "../include/ata.h"

#define BIONIC_INSTALL_LBA 3000

#define INSTALL_MAGIC_0 'B'
#define INSTALL_MAGIC_1 'I'
#define INSTALL_MAGIC_2 'O'
#define INSTALL_MAGIC_3 'N'
#define INSTALL_MAGIC_4 'I'
#define INSTALL_MAGIC_5 'C'
#define INSTALL_MAGIC_6 'S'
#define INSTALL_MAGIC_7 'T'

#define INSTALL_VERSION 1

typedef struct {
    char magic[8];
    uint32_t version;
    uint32_t fs_mode;
    uint32_t flags;
} bionic_install_config_t;

static bionic_fs_mode_t current_fs_mode = BIONIC_FS_MODE_RAM;
static uint8_t install_loaded = 0;

static void install_memclear(uint8_t* ptr, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        ptr[i] = 0;
    }
}

void install_init(void) {
    current_fs_mode = BIONIC_FS_MODE_RAM;
    install_loaded = 0;
}

static int install_validate_config(bionic_install_config_t* cfg) {
    if (!cfg) {
        return -1;
    }

    if (cfg->magic[0] != INSTALL_MAGIC_0 ||
        cfg->magic[1] != INSTALL_MAGIC_1 ||
        cfg->magic[2] != INSTALL_MAGIC_2 ||
        cfg->magic[3] != INSTALL_MAGIC_3 ||
        cfg->magic[4] != INSTALL_MAGIC_4 ||
        cfg->magic[5] != INSTALL_MAGIC_5 ||
        cfg->magic[6] != INSTALL_MAGIC_6 ||
        cfg->magic[7] != INSTALL_MAGIC_7) {
        return -1;
    }

    if (cfg->version != INSTALL_VERSION) {
        return -1;
    }

    if (cfg->fs_mode > BIONIC_FS_MODE_FAT32) {
        return -1;
    }

    return 0;
}

int install_config_clear(void) {
    uint8_t sector[ATA_SECTOR_SIZE];

    if (!ata_is_present()) {
        return -1;
    }

    install_memclear(sector, ATA_SECTOR_SIZE);

    if (ata_write_sector(BIONIC_INSTALL_LBA, sector) != 0) {
        return -1;
    }

    current_fs_mode = BIONIC_FS_MODE_RAM;
    install_loaded = 0;

    return 0;
}

int install_config_exists(void) {
    uint8_t sector[ATA_SECTOR_SIZE];

    if (!ata_is_present()) {
        return 0;
    }

    if (ata_read_sector(BIONIC_INSTALL_LBA, sector) != 0) {
        return 0;
    }

    bionic_install_config_t* cfg = (bionic_install_config_t*)sector;

    return install_validate_config(cfg) == 0;
}

int install_config_load(void) {
    uint8_t sector[ATA_SECTOR_SIZE];

    if (!ata_is_present()) {
        return -1;
    }

    if (ata_read_sector(BIONIC_INSTALL_LBA, sector) != 0) {
        return -1;
    }

    bionic_install_config_t* cfg = (bionic_install_config_t*)sector;

    if (install_validate_config(cfg) != 0) {
        return -1;
    }

    current_fs_mode = (bionic_fs_mode_t)cfg->fs_mode;
    install_loaded = 1;

    return 0;
}

int install_config_save(bionic_fs_mode_t mode) {
    uint8_t sector[ATA_SECTOR_SIZE];

    if (!ata_is_present()) {
        return -1;
    }

    install_memclear(sector, ATA_SECTOR_SIZE);

    bionic_install_config_t* cfg = (bionic_install_config_t*)sector;

    cfg->magic[0] = INSTALL_MAGIC_0;
    cfg->magic[1] = INSTALL_MAGIC_1;
    cfg->magic[2] = INSTALL_MAGIC_2;
    cfg->magic[3] = INSTALL_MAGIC_3;
    cfg->magic[4] = INSTALL_MAGIC_4;
    cfg->magic[5] = INSTALL_MAGIC_5;
    cfg->magic[6] = INSTALL_MAGIC_6;
    cfg->magic[7] = INSTALL_MAGIC_7;

    cfg->version = INSTALL_VERSION;
    cfg->fs_mode = (uint32_t)mode;
    cfg->flags = 0;

    if (ata_write_sector(BIONIC_INSTALL_LBA, sector) != 0) {
        return -1;
    }

    current_fs_mode = mode;
    install_loaded = 1;

    return 0;
}

bionic_fs_mode_t install_get_fs_mode(void) {
    return current_fs_mode;
}

const char* install_fs_mode_name(bionic_fs_mode_t mode) {
    if (mode == BIONIC_FS_MODE_RAM) {
        return "RAM only";
    }

    if (mode == BIONIC_FS_MODE_BIONICFS) {
        return "BIONICFS";
    }

    if (mode == BIONIC_FS_MODE_FAT32) {
        return "FAT32 experimental";
    }

    return "Unknown";
}