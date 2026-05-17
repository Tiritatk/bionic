#ifndef BIONICFS_H
#define BIONICFS_H

#include <stdint.h>

int bionicfs_save(void);
int bionicfs_load(void);

uint32_t bionicfs_start_lba(void);
int bionicfs_probe(uint32_t* entry_count);

#endif