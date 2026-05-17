#ifndef INSTALL_H
#define INSTALL_H

#include <stdint.h>

typedef enum {
    BIONIC_FS_MODE_RAM = 0,
    BIONIC_FS_MODE_BIONICFS = 1,
    BIONIC_FS_MODE_FAT32 = 2
} bionic_fs_mode_t;

void install_init(void);

int install_config_exists(void);
int install_config_load(void);
int install_config_save(bionic_fs_mode_t mode);
int install_config_clear(void);

bionic_fs_mode_t install_get_fs_mode(void);

const char* install_fs_mode_name(bionic_fs_mode_t mode);

#endif