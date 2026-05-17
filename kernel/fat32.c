#include "../include/fat32.h"
#include "../include/ata.h"
#include "../include/fs.h"

typedef struct {
    uint8_t mounted;
    uint8_t drive;

    uint32_t partition_lba;

    uint32_t bytes_per_sector;
    uint32_t sectors_per_cluster;
    uint32_t reserved_sectors;
    uint32_t num_fats;
    uint32_t fat_size;
    uint32_t total_sectors;
    uint32_t root_cluster;

    uint32_t fat_start_lba;
    uint32_t data_start_lba;
} fat32_state_t;

static fat32_state_t fat32;

static uint16_t fat32_read_u16(const uint8_t* b, uint32_t off) {
    return (uint16_t)b[off] | ((uint16_t)b[off + 1] << 8);
}

static uint32_t fat32_read_u32(const uint8_t* b, uint32_t off) {
    return ((uint32_t)b[off]) |
           ((uint32_t)b[off + 1] << 8) |
           ((uint32_t)b[off + 2] << 16) |
           ((uint32_t)b[off + 3] << 24);
}

static int fat32_parse_boot_sector(uint8_t drive, uint32_t lba) {
    uint8_t sector[ATA_SECTOR_SIZE];

    if (ata_read_sector_drive(drive, lba, sector) != 0) {
        return -1;
    }

    if (sector[510] != 0x55 || sector[511] != 0xAA) {
        return -1;
    }

    uint32_t bytes_per_sector = fat32_read_u16(sector, 11);
    uint32_t sectors_per_cluster = sector[13];
    uint32_t reserved_sectors = fat32_read_u16(sector, 14);
    uint32_t num_fats = sector[16];
    uint32_t total_sectors_16 = fat32_read_u16(sector, 19);
    uint32_t total_sectors_32 = fat32_read_u32(sector, 32);
    uint32_t fat_size_32 = fat32_read_u32(sector, 36);
    uint32_t root_cluster = fat32_read_u32(sector, 44);

    uint32_t total_sectors = total_sectors_16 != 0 ? total_sectors_16 : total_sectors_32;

    if (bytes_per_sector != 512) {
        return -1;
    }

    if (sectors_per_cluster == 0) {
        return -1;
    }

    if (reserved_sectors == 0) {
        return -1;
    }

    if (num_fats == 0) {
        return -1;
    }

    if (fat_size_32 == 0) {
        return -1;
    }

    if (root_cluster < 2) {
        return -1;
    }

    fat32.mounted = 1;
    fat32.drive = drive;
    fat32.partition_lba = lba;

    fat32.bytes_per_sector = bytes_per_sector;
    fat32.sectors_per_cluster = sectors_per_cluster;
    fat32.reserved_sectors = reserved_sectors;
    fat32.num_fats = num_fats;
    fat32.fat_size = fat_size_32;
    fat32.total_sectors = total_sectors;
    fat32.root_cluster = root_cluster;

    fat32.fat_start_lba = lba + reserved_sectors;
    fat32.data_start_lba = lba + reserved_sectors + num_fats * fat_size_32;

    return 0;
}

static void fat32_memclear(char* buf, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        buf[i] = 0;
    }
}

static uint32_t fat32_cluster_to_lba(uint32_t cluster) {
    /*
       FAT32 cluster 2 empieza en data_start_lba.
    */
    return fat32.data_start_lba +
           ((cluster - 2) * fat32.sectors_per_cluster);
}

static uint32_t fat32_next_cluster(uint32_t cluster) {
    uint8_t sector[ATA_SECTOR_SIZE];

    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fat32.fat_start_lba + (fat_offset / ATA_SECTOR_SIZE);
    uint32_t ent_offset = fat_offset % ATA_SECTOR_SIZE;

    if (ata_read_sector_drive(fat32.drive, fat_sector, sector) != 0) {
        return 0x0FFFFFFF;
    }

    uint32_t value = fat32_read_u32(sector, ent_offset);

    /*
       En FAT32 solo cuentan 28 bits.
    */
    return value & 0x0FFFFFFF;
}

static int fat32_is_end_cluster(uint32_t cluster) {
    return cluster >= 0x0FFFFFF8;
}

static void fat32_make_short_name(const uint8_t* entry, char* out, uint32_t max) {
    fat32_memclear(out, max);

    if (max == 0) {
        return;
    }

    uint32_t pos = 0;

    /*
       Nombre base: bytes 0-7.
    */
    for (uint32_t i = 0; i < 8; i++) {
        if (entry[i] == ' ') {
            break;
        }

        if (pos < max - 1) {
            out[pos++] = entry[i];
        }
    }

    /*
       Extension: bytes 8-10.
    */
    if (entry[8] != ' ') {
        if (pos < max - 1) {
            out[pos++] = '.';
        }

        for (uint32_t i = 8; i < 11; i++) {
            if (entry[i] == ' ') {
                break;
            }

            if (pos < max - 1) {
                out[pos++] = entry[i];
            }
        }
    }

    out[pos] = 0;
}

static void fat32_add_prefix_name(const char* prefix, const char* name, char* out, uint32_t max) {
    fat32_memclear(out, max);

    uint32_t pos = 0;
    uint32_t i = 0;

    while (prefix && prefix[i] && pos < max - 1) {
        out[pos++] = prefix[i++];
    }

    i = 0;

    while (name && name[i] && pos < max - 1) {
        out[pos++] = name[i++];
    }

    out[pos] = 0;
}

static int fat32_char_upper(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 32;
    }

    return c;
}

static int fat32_streq_ignore_case(const char* a, const char* b) {
    uint32_t i = 0;

    while (a[i] && b[i]) {
        if (fat32_char_upper(a[i]) != fat32_char_upper(b[i])) {
            return 0;
        }

        i++;
    }

    return a[i] == b[i];
}

static uint32_t fat32_file_first_cluster(const uint8_t* entry) {
    uint32_t high = fat32_read_u16(entry, 20);
    uint32_t low = fat32_read_u16(entry, 26);

    return (high << 16) | low;
}

static uint32_t fat32_file_size(const uint8_t* entry) {
    return fat32_read_u32(entry, 28);
}

static void fat32_print_text_buffer(const uint8_t* data,
                                    uint32_t size,
                                    void (*print_func)(const char*)) {
    char line[96];
    uint32_t pos = 0;

    for (uint32_t i = 0; i < 96; i++) {
        line[i] = 0;
    }

    for (uint32_t i = 0; i < size; i++) {
        char c = (char)data[i];

        if (c == '\r') {
            continue;
        }

        if (c == '\n') {
            line[pos] = 0;

            if (pos > 0) {
                print_func(line);
            } else {
                print_func("");
            }

            pos = 0;

            for (uint32_t j = 0; j < 96; j++) {
                line[j] = 0;
            }

            continue;
        }

        if (c < 32 || c > 126) {
            c = '.';
        }

        line[pos++] = c;

        if (pos >= 95) {
            line[pos] = 0;
            print_func(line);

            pos = 0;

            for (uint32_t j = 0; j < 96; j++) {
                line[j] = 0;
            }
        }
    }

    if (pos > 0) {
        line[pos] = 0;
        print_func(line);
    }
}

int fat32_mount(uint8_t drive) {
    fat32.mounted = 0;

    if (!ata_is_present_drive(drive)) {
        return -1;
    }

    /*
       Primera versión:
       intentamos montar FAT32 directamente en LBA 0.
       Esto funciona con imágenes tipo "superfloppy":
       mkfs.fat -F 32 fat_disk.img
    */
    if (fat32_parse_boot_sector(drive, 0) == 0) {
        return 0;
    }

    /*
       Más adelante añadiremos MBR/particiones aquí.
    */
    return -1;
}

uint8_t fat32_is_mounted(void) {
    return fat32.mounted;
}

uint8_t fat32_drive(void) {
    return fat32.drive;
}

uint32_t fat32_total_sectors(void) {
    return fat32.total_sectors;
}

uint32_t fat32_fat_start_lba(void) {
    return fat32.fat_start_lba;
}

uint32_t fat32_data_start_lba(void) {
    return fat32.data_start_lba;
}

uint32_t fat32_root_cluster(void) {
    return fat32.root_cluster;
}

uint32_t fat32_sectors_per_cluster(void) {
    return fat32.sectors_per_cluster;
}

uint32_t fat32_bytes_per_sector(void) {
    return fat32.bytes_per_sector;
}

uint32_t fat32_reserved_sectors(void) {
    return fat32.reserved_sectors;
}

uint32_t fat32_num_fats(void) {
    return fat32.num_fats;
}

uint32_t fat32_fat_size(void) {
    return fat32.fat_size;
}

int fat32_list_root(void (*print_func)(const char*)) {
    if (!fat32.mounted) {
        return -1;
    }

    if (!print_func) {
        return -1;
    }

    uint32_t cluster = fat32.root_cluster;
    uint8_t sector[ATA_SECTOR_SIZE];

    while (!fat32_is_end_cluster(cluster)) {
        uint32_t cluster_lba = fat32_cluster_to_lba(cluster);

        for (uint32_t s = 0; s < fat32.sectors_per_cluster; s++) {
            if (ata_read_sector_drive(fat32.drive, cluster_lba + s, sector) != 0) {
                return -1;
            }

            /*
               512 / 32 = 16 directory entries por sector.
            */
            for (uint32_t off = 0; off < ATA_SECTOR_SIZE; off += 32) {
                uint8_t first = sector[off];

                /*
                   0x00 = no hay más entradas.
                */
                if (first == 0x00) {
                    return 0;
                }

                /*
                   0xE5 = entrada borrada.
                */
                if (first == 0xE5) {
                    continue;
                }

                uint8_t attr = sector[off + 11];

                /*
                   0x0F = Long File Name.
                   Por ahora lo ignoramos.
                */
                if (attr == 0x0F) {
                    continue;
                }

                /*
                   Ignorar volume label.
                */
                if (attr & 0x08) {
                    continue;
                }

                char name[64];
                char line[96];

                fat32_make_short_name(&sector[off], name, 64);

                if (name[0] == 0) {
                    continue;
                }

                if (attr & 0x10) {
                    fat32_add_prefix_name("[DIR]  ", name, line, 96);
                } else {
                    fat32_add_prefix_name("[FILE] ", name, line, 96);
                }

                print_func(line);
            }
        }

        cluster = fat32_next_cluster(cluster);
    }

    return 0;
}

int fat32_read_root_file(const char* filename, void (*print_func)(const char*)) {
    if (!fat32.mounted) {
        return -1;
    }

    if (!filename || !print_func) {
        return -1;
    }

    uint32_t cluster = fat32.root_cluster;
    uint8_t sector[ATA_SECTOR_SIZE];

    while (!fat32_is_end_cluster(cluster)) {
        uint32_t cluster_lba = fat32_cluster_to_lba(cluster);

        for (uint32_t s = 0; s < fat32.sectors_per_cluster; s++) {
            if (ata_read_sector_drive(fat32.drive, cluster_lba + s, sector) != 0) {
                return -1;
            }

            for (uint32_t off = 0; off < ATA_SECTOR_SIZE; off += 32) {
                uint8_t first = sector[off];

                if (first == 0x00) {
                    return -1;
                }

                if (first == 0xE5) {
                    continue;
                }

                uint8_t attr = sector[off + 11];

                /*
                   Ignorar long file name y volume label.
                */
                if (attr == 0x0F) {
                    continue;
                }

                if (attr & 0x08) {
                    continue;
                }

                /*
                   Ignorar directorios para fatcat.
                */
                if (attr & 0x10) {
                    continue;
                }

                char name[64];
                fat32_make_short_name(&sector[off], name, 64);

                if (!fat32_streq_ignore_case(name, filename)) {
                    continue;
                }

                uint32_t file_cluster = fat32_file_first_cluster(&sector[off]);
                uint32_t file_size = fat32_file_size(&sector[off]);

                if (file_cluster < 2) {
                    print_func("[empty file]");
                    return 0;
                }

                if (file_size == 0) {
                    print_func("[empty file]");
                    return 0;
                }

                uint32_t remaining = file_size;
                uint32_t current_cluster = file_cluster;

                uint8_t data_sector[ATA_SECTOR_SIZE];

                while (!fat32_is_end_cluster(current_cluster) && remaining > 0) {
                    uint32_t data_lba = fat32_cluster_to_lba(current_cluster);

                    for (uint32_t ds = 0;
                         ds < fat32.sectors_per_cluster && remaining > 0;
                         ds++) {
                        if (ata_read_sector_drive(fat32.drive, data_lba + ds, data_sector) != 0) {
                            return -1;
                        }

                        uint32_t to_print = remaining;

                        if (to_print > ATA_SECTOR_SIZE) {
                            to_print = ATA_SECTOR_SIZE;
                        }

                        fat32_print_text_buffer(data_sector, to_print, print_func);

                        remaining -= to_print;
                    }

                    current_cluster = fat32_next_cluster(current_cluster);
                }

                return 0;
            }
        }

        cluster = fat32_next_cluster(cluster);
    }

    return -1;
}

static int fat32_read_root_file_to_buffer(const char* filename,
                                          char* out,
                                          uint32_t max_size,
                                          uint32_t* out_size) {
    if (!fat32.mounted) {
        return -1;
    }

    if (!filename || !out || max_size == 0) {
        return -1;
    }

    for (uint32_t i = 0; i < max_size; i++) {
        out[i] = 0;
    }

    if (out_size) {
        *out_size = 0;
    }

    uint32_t cluster = fat32.root_cluster;
    uint8_t sector[ATA_SECTOR_SIZE];

    while (!fat32_is_end_cluster(cluster)) {
        uint32_t cluster_lba = fat32_cluster_to_lba(cluster);

        for (uint32_t s = 0; s < fat32.sectors_per_cluster; s++) {
            if (ata_read_sector_drive(fat32.drive, cluster_lba + s, sector) != 0) {
                return -1;
            }

            for (uint32_t off = 0; off < ATA_SECTOR_SIZE; off += 32) {
                uint8_t first = sector[off];

                if (first == 0x00) {
                    return -1;
                }

                if (first == 0xE5) {
                    continue;
                }

                uint8_t attr = sector[off + 11];

                /*
                   Ignorar LFN y volume label.
                */
                if (attr == 0x0F) {
                    continue;
                }

                if (attr & 0x08) {
                    continue;
                }

                /*
                   fatimport solo importa archivos, no carpetas.
                */
                if (attr & 0x10) {
                    continue;
                }

                char name[64];
                fat32_make_short_name(&sector[off], name, 64);

                if (!fat32_streq_ignore_case(name, filename)) {
                    continue;
                }

                uint32_t file_cluster = fat32_file_first_cluster(&sector[off]);
                uint32_t file_size = fat32_file_size(&sector[off]);

                if (file_size == 0 || file_cluster < 2) {
                    if (out_size) {
                        *out_size = 0;
                    }

                    return 0;
                }

                /*
                   De momento limitamos al tamaño máximo del buffer.
                   Dejamos 1 byte para \0.
                */
                uint32_t limit = max_size - 1;
                uint32_t remaining = file_size;

                if (remaining > limit) {
                    remaining = limit;
                }

                uint32_t written = 0;
                uint32_t current_cluster = file_cluster;

                uint8_t data_sector[ATA_SECTOR_SIZE];

                while (!fat32_is_end_cluster(current_cluster) && written < remaining) {
                    uint32_t data_lba = fat32_cluster_to_lba(current_cluster);

                    for (uint32_t ds = 0;
                         ds < fat32.sectors_per_cluster && written < remaining;
                         ds++) {
                        if (ata_read_sector_drive(fat32.drive, data_lba + ds, data_sector) != 0) {
                            return -1;
                        }

                        uint32_t to_copy = remaining - written;

                        if (to_copy > ATA_SECTOR_SIZE) {
                            to_copy = ATA_SECTOR_SIZE;
                        }

                        for (uint32_t i = 0; i < to_copy; i++) {
                            out[written++] = (char)data_sector[i];
                        }
                    }

                    current_cluster = fat32_next_cluster(current_cluster);
                }

                out[written] = 0;

                if (out_size) {
                    *out_size = written;
                }

                return 0;
            }
        }

        cluster = fat32_next_cluster(cluster);
    }

    return -1;
}

int fat32_import_root_file(const char* fat_filename, const char* ramfs_path) {
    if (!fat32.mounted) {
        return -1;
    }

    if (!fat_filename || !ramfs_path) {
        return -1;
    }

    /*
       Tu RAMFS actualmente parece orientado a archivos pequeños de texto.
       Usamos 512 bytes para encajar con el editor/write actual.
       Si tu FS soporta más, luego subimos este límite.
    */
    char buffer[512];
    uint32_t size = 0;

    if (fat32_read_root_file_to_buffer(fat_filename, buffer, 512, &size) != 0) {
        return -1;
    }

    /*
       Crear archivo destino si no existe.
    */
    fs_touch(ramfs_path);

    /*
       Escribir contenido.
       Para archivos vacíos, escribimos string vacío.
    */
    if (fs_write(ramfs_path, buffer) != 0) {
        return -1;
    }

    return 0;
}