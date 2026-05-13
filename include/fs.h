#ifndef FS_H
#define FS_H

#include <stdint.h>

typedef enum {
    FS_FILE,
    FS_DIR
} fs_node_type_t;

typedef struct fs_node {
    char name[32];
    fs_node_type_t type;

    struct fs_node* parent;
    struct fs_node* children;
    struct fs_node* next;

    char content[512];
    uint32_t size;
} fs_node_t;

void fs_init(void);

void fs_ls(const char* path);
void fs_pwd(void);
void fs_print_current_path(void);

int fs_mkdir(const char* name);
    int fs_mkdir_p(const char* path);
int fs_touch(const char* name);
int fs_cd(const char* name);
int fs_write(const char* name, const char* text);
int fs_cat(const char* name);
int fs_rename(const char* path, const char* new_name);
int fs_mv(const char* src_path, const char* dst_path);
int fs_cp(const char* src_path, const char* dst_path);
int fs_rm(const char* name);
int fs_rm_recursive(const char* path);
int fs_rmdir(const char* name);
int fs_append(const char* name, const char* text);
int fs_clearfile(const char* name);
void fs_tree(const char* path);
void fs_find(const char* start_path, const char* query);
fs_node_t* fs_get_root(void);
fs_node_t* fs_get_current(void);
void fs_set_current(fs_node_t* node);
uint32_t fs_count_children(fs_node_t* dir);
fs_node_t* fs_get_child_at(fs_node_t* dir, uint32_t index);
fs_node_t* fs_get_node(const char* path);
#endif