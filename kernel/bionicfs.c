#include "../include/bionicfs.h"
#include "../include/ata.h"
#include "../include/fs.h"
#include "../include/kprintf.h"

#define BIONICFS_START_LBA 4096
#define BIONICFS_MAGIC_0 'B'
#define BIONICFS_MAGIC_1 'I'
#define BIONICFS_MAGIC_2 'O'
#define BIONICFS_MAGIC_3 'N'
#define BIONICFS_MAGIC_4 'I'
#define BIONICFS_MAGIC_5 'C'
#define BIONICFS_MAGIC_6 'F'
#define BIONICFS_MAGIC_7 'S'

#define BIONICFS_VERSION 1
#define BIONICFS_MAX_PATH 128

typedef struct {
    char magic[8];
    uint32_t version;
    uint32_t entry_count;
} bionicfs_header_t;

typedef struct {
    uint32_t current_lba;
    uint32_t offset;
    uint8_t sector[ATA_SECTOR_SIZE];
} bionicfs_writer_t;

typedef struct {
    uint32_t current_lba;
    uint32_t offset;
    uint8_t sector[ATA_SECTOR_SIZE];
} bionicfs_reader_t;

static void bionicfs_memclear(uint8_t* ptr, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        ptr[i] = 0;
    }
}

static uint32_t bionicfs_strlen(const char* s) {
    uint32_t i = 0;

    while (s && s[i]) {
        i++;
    }

    return i;
}

static void bionicfs_strcopy(char* dst, const char* src, uint32_t max) {
    if (!dst || max == 0) {
        return;
    }

    for (uint32_t i = 0; i < max; i++) {
        dst[i] = 0;
    }

    if (!src) {
        return;
    }

    uint32_t i = 0;

    while (src[i] && i < max - 1) {
        dst[i] = src[i];
        i++;
    }

    dst[i] = 0;
}

static void bionicfs_build_child_path(const char* parent, const char* name, char* out, uint32_t max) {
    if (!out || max == 0) {
        return;
    }

    for (uint32_t i = 0; i < max; i++) {
        out[i] = 0;
    }

    uint32_t pos = 0;

    if (!parent || parent[0] == '\0') {
        out[pos++] = '/';
    } else {
        uint32_t i = 0;

        while (parent[i] && pos < max - 1) {
            out[pos++] = parent[i++];
        }
    }

    if (!(pos == 1 && out[0] == '/')) {
        if (pos < max - 1) {
            out[pos++] = '/';
        }
    }

    uint32_t j = 0;

    while (name && name[j] && pos < max - 1) {
        out[pos++] = name[j++];
    }

    out[pos] = 0;
}

static uint32_t bionicfs_count_nodes(fs_node_t* dir) {
    if (!dir || dir->type != FS_DIR) {
        return 0;
    }

    uint32_t count = 0;
    uint32_t children = fs_count_children(dir);

    for (uint32_t i = 0; i < children; i++) {
        fs_node_t* child = fs_get_child_at(dir, i);

        if (!child) {
            continue;
        }

        count++;

        if (child->type == FS_DIR) {
            count += bionicfs_count_nodes(child);
        }
    }

    return count;
}

static int bionicfs_writer_flush(bionicfs_writer_t* w) {
    if (!w) {
        return -1;
    }

    if (ata_write_sector(w->current_lba, w->sector) != 0) {
        return -1;
    }

    bionicfs_memclear(w->sector, ATA_SECTOR_SIZE);
    w->current_lba++;
    w->offset = 0;

    return 0;
}

static void bionicfs_writer_init(bionicfs_writer_t* w, uint32_t start_lba) {
    w->current_lba = start_lba;
    w->offset = 0;
    bionicfs_memclear(w->sector, ATA_SECTOR_SIZE);
}

static int bionicfs_write_byte(bionicfs_writer_t* w, uint8_t value) {
    if (!w) {
        return -1;
    }

    w->sector[w->offset++] = value;

    if (w->offset >= ATA_SECTOR_SIZE) {
        return bionicfs_writer_flush(w);
    }

    return 0;
}

static int bionicfs_write_data(bionicfs_writer_t* w, const uint8_t* data, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        if (bionicfs_write_byte(w, data[i]) != 0) {
            return -1;
        }
    }

    return 0;
}

static int bionicfs_write_u32(bionicfs_writer_t* w, uint32_t value) {
    if (bionicfs_write_byte(w, (uint8_t)(value & 0xFF)) != 0) return -1;
    if (bionicfs_write_byte(w, (uint8_t)((value >> 8) & 0xFF)) != 0) return -1;
    if (bionicfs_write_byte(w, (uint8_t)((value >> 16) & 0xFF)) != 0) return -1;
    if (bionicfs_write_byte(w, (uint8_t)((value >> 24) & 0xFF)) != 0) return -1;

    return 0;
}

static int bionicfs_write_entry(bionicfs_writer_t* w, fs_node_t* node, const char* path) {
    if (!w || !node || !path) {
        return -1;
    }

    uint32_t path_len = bionicfs_strlen(path);
    uint32_t size = 0;

    if (node->type == FS_FILE) {
        size = node->size;
    }

    /*
       Entry format:
       u8 type
       u32 path_len
       u32 size
       path bytes
       content bytes if file
    */

    if (bionicfs_write_byte(w, node->type) != 0) return -1;
    if (bionicfs_write_u32(w, path_len) != 0) return -1;
    if (bionicfs_write_u32(w, size) != 0) return -1;

    if (bionicfs_write_data(w, (const uint8_t*)path, path_len) != 0) {
        return -1;
    }

    if (node->type == FS_FILE && size > 0) {
        if (bionicfs_write_data(w, (const uint8_t*)node->content, size) != 0) {
            return -1;
        }
    }

    return 0;
}

static int bionicfs_save_node_recursive(bionicfs_writer_t* w, fs_node_t* dir, const char* parent_path) {
    if (!w || !dir || dir->type != FS_DIR) {
        return -1;
    }

    uint32_t children = fs_count_children(dir);

    for (uint32_t i = 0; i < children; i++) {
        fs_node_t* child = fs_get_child_at(dir, i);

        if (!child) {
            continue;
        }

        char child_path[BIONICFS_MAX_PATH];
        bionicfs_build_child_path(parent_path, child->name, child_path, BIONICFS_MAX_PATH);

        if (bionicfs_write_entry(w, child, child_path) != 0) {
            return -1;
        }

        if (child->type == FS_DIR) {
            if (bionicfs_save_node_recursive(w, child, child_path) != 0) {
                return -1;
            }
        }
    }

    return 0;
}

static void bionicfs_reader_init(bionicfs_reader_t* r, uint32_t start_lba) {
    r->current_lba = start_lba;
    r->offset = 0;
    bionicfs_memclear(r->sector, ATA_SECTOR_SIZE);

    ata_read_sector(r->current_lba, r->sector);
}

static int bionicfs_reader_next_sector(bionicfs_reader_t* r) {
    r->current_lba++;
    r->offset = 0;
    bionicfs_memclear(r->sector, ATA_SECTOR_SIZE);

    if (ata_read_sector(r->current_lba, r->sector) != 0) {
        return -1;
    }

    return 0;
}

static int bionicfs_read_byte(bionicfs_reader_t* r, uint8_t* out) {
    if (!r || !out) {
        return -1;
    }

    *out = r->sector[r->offset++];

    if (r->offset >= ATA_SECTOR_SIZE) {
        if (bionicfs_reader_next_sector(r) != 0) {
            return -1;
        }
    }

    return 0;
}

static int bionicfs_read_data(bionicfs_reader_t* r, uint8_t* out, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        if (bionicfs_read_byte(r, &out[i]) != 0) {
            return -1;
        }
    }

    return 0;
}

static int bionicfs_read_u32(bionicfs_reader_t* r, uint32_t* out) {
    uint8_t b0, b1, b2, b3;

    if (bionicfs_read_byte(r, &b0) != 0) return -1;
    if (bionicfs_read_byte(r, &b1) != 0) return -1;
    if (bionicfs_read_byte(r, &b2) != 0) return -1;
    if (bionicfs_read_byte(r, &b3) != 0) return -1;

    *out = ((uint32_t)b0) |
           ((uint32_t)b1 << 8) |
           ((uint32_t)b2 << 16) |
           ((uint32_t)b3 << 24);

    return 0;
}

int bionicfs_save(void) {
    fs_node_t* root = fs_get_root();

    if (!root) {
        return -1;
    }

    uint32_t entry_count = bionicfs_count_nodes(root);

    bionicfs_header_t header;

    header.magic[0] = BIONICFS_MAGIC_0;
    header.magic[1] = BIONICFS_MAGIC_1;
    header.magic[2] = BIONICFS_MAGIC_2;
    header.magic[3] = BIONICFS_MAGIC_3;
    header.magic[4] = BIONICFS_MAGIC_4;
    header.magic[5] = BIONICFS_MAGIC_5;
    header.magic[6] = BIONICFS_MAGIC_6;
    header.magic[7] = BIONICFS_MAGIC_7;
    header.version = BIONICFS_VERSION;
    header.entry_count = entry_count;

    uint8_t header_sector[ATA_SECTOR_SIZE];
    bionicfs_memclear(header_sector, ATA_SECTOR_SIZE);

    uint8_t* h = (uint8_t*)&header;

    for (uint32_t i = 0; i < sizeof(bionicfs_header_t); i++) {
        header_sector[i] = h[i];
    }

    if (ata_write_sector(BIONICFS_START_LBA, header_sector) != 0) {
        return -1;
    }

    bionicfs_writer_t writer;
    bionicfs_writer_init(&writer, BIONICFS_START_LBA + 1);

    if (bionicfs_save_node_recursive(&writer, root, "/") != 0) {
        return -1;
    }

    /*
       Flush último sector aunque no esté lleno.
    */
    if (writer.offset > 0) {
        if (bionicfs_writer_flush(&writer) != 0) {
            return -1;
        }
    }

    return 0;
}

int bionicfs_load(void) {
    uint8_t header_sector[ATA_SECTOR_SIZE];

    if (ata_read_sector(BIONICFS_START_LBA, header_sector) != 0) {
        return -1;
    }

    bionicfs_header_t* header = (bionicfs_header_t*)header_sector;

    if (header->magic[0] != BIONICFS_MAGIC_0 ||
        header->magic[1] != BIONICFS_MAGIC_1 ||
        header->magic[2] != BIONICFS_MAGIC_2 ||
        header->magic[3] != BIONICFS_MAGIC_3 ||
        header->magic[4] != BIONICFS_MAGIC_4 ||
        header->magic[5] != BIONICFS_MAGIC_5 ||
        header->magic[6] != BIONICFS_MAGIC_6 ||
        header->magic[7] != BIONICFS_MAGIC_7) {
        return -1;
    }

    if (header->version != BIONICFS_VERSION) {
        return -1;
    }

    fs_reset();

    bionicfs_reader_t reader;
    bionicfs_reader_init(&reader, BIONICFS_START_LBA + 1);

    for (uint32_t entry = 0; entry < header->entry_count; entry++) {
        uint8_t type;
        uint32_t path_len;
        uint32_t size;

        if (bionicfs_read_byte(&reader, &type) != 0) return -1;
        if (bionicfs_read_u32(&reader, &path_len) != 0) return -1;
        if (bionicfs_read_u32(&reader, &size) != 0) return -1;

        if (path_len >= BIONICFS_MAX_PATH) {
            return -1;
        }

        char path[BIONICFS_MAX_PATH];

        for (uint32_t i = 0; i < BIONICFS_MAX_PATH; i++) {
            path[i] = 0;
        }

        if (bionicfs_read_data(&reader, (uint8_t*)path, path_len) != 0) {
            return -1;
        }

        path[path_len] = 0;

        if (type == FS_DIR) {
            fs_mkdir(path);
        } else if (type == FS_FILE) {
            fs_touch(path);

            if (size > 0) {
                char content[512];

                for (uint32_t i = 0; i < 512; i++) {
                    content[i] = 0;
                }

                uint32_t read_size = size;

                if (read_size > 511) {
                    read_size = 511;
                }

                if (bionicfs_read_data(&reader, (uint8_t*)content, read_size) != 0) {
                    return -1;
                }

                /*
                   Si el archivo guardado era mayor que 511,
                   consumimos el resto aunque no lo carguemos.
                */
                for (uint32_t i = read_size; i < size; i++) {
                    uint8_t dummy;
                    if (bionicfs_read_byte(&reader, &dummy) != 0) {
                        return -1;
                    }
                }

                fs_write(path, content);
            }
        } else {
            return -1;
        }
    }

    return 0;
}

uint32_t bionicfs_start_lba(void) {
    return BIONICFS_START_LBA;
}

int bionicfs_probe(uint32_t* entry_count) {
    uint8_t header_sector[ATA_SECTOR_SIZE];

    if (ata_read_sector(BIONICFS_START_LBA, header_sector) != 0) {
        return -1;
    }

    bionicfs_header_t* header = (bionicfs_header_t*)header_sector;

    if (header->magic[0] != BIONICFS_MAGIC_0 ||
        header->magic[1] != BIONICFS_MAGIC_1 ||
        header->magic[2] != BIONICFS_MAGIC_2 ||
        header->magic[3] != BIONICFS_MAGIC_3 ||
        header->magic[4] != BIONICFS_MAGIC_4 ||
        header->magic[5] != BIONICFS_MAGIC_5 ||
        header->magic[6] != BIONICFS_MAGIC_6 ||
        header->magic[7] != BIONICFS_MAGIC_7) {
        return -1;
    }

    if (header->version != BIONICFS_VERSION) {
        return -1;
    }

    if (entry_count) {
        *entry_count = header->entry_count;
    }

    return 0;
}