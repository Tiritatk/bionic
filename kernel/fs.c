#include "fs.h"
#include "pmm.h"
#include "kprintf.h"
#include "kmalloc.h"

static fs_node_t* fs_root = 0;
static fs_node_t* fs_current = 0;

// static fs_node_t* fs_resolve_path(const char* path);

static void fs_clear_node(fs_node_t* node) {
    uint8_t* ptr = (uint8_t*)node;

    for (uint32_t i = 0; i < sizeof(fs_node_t); i++) {
        ptr[i] = 0;
    }
}

static fs_node_t* fs_alloc_node(void) {
    fs_node_t* node = (fs_node_t*)kmalloc(sizeof(fs_node_t));

    if (!node) {
        return 0;
    }

    fs_clear_node(node);
    return node;
}

static void fs_copy_name(char* dst, const char* src) {
    uint32_t i = 0;

    while (src[i] && i < 31) {
        dst[i] = src[i];
        i++;
    }

    dst[i] = '\0';
}

static int fs_name_equals(const char* a, const char* b) {
    uint32_t i = 0;

    while (a[i] && b[i]) {
        if (a[i] != b[i]) {
            return 0;
        }

        i++;
    }

    return a[i] == b[i];
}

static fs_node_t* fs_find_child(fs_node_t* dir, const char* name) {
    if (!dir || dir->type != FS_DIR) {
        return 0;
    }

    fs_node_t* child = dir->children;

    while (child) {
        if (fs_name_equals(child->name, name)) {
            return child;
        }

        child = child->next;
    }

    return 0;
}

static void fs_add_child(fs_node_t* parent, fs_node_t* child) {
    child->parent = parent;

    if (!parent->children) {
        parent->children = child;
        return;
    }

    fs_node_t* current = parent->children;

    while (current->next) {
        current = current->next;
    }

    current->next = child;
}

/* Copia una parte de una ruta a un nombre corto de 31 caracteres. */
static void fs_copy_part(char* dst, const char* src, uint32_t start, uint32_t end) {
    uint32_t j = 0;

    for (uint32_t i = start; i < end && j < 31; i++) {
        dst[j++] = src[i];
    }

    dst[j] = '\0';
}

/*
 * Resuelve una ruta existente.
 * Soporta:
 *   /home/docs
 *   home/docs
 *   ./archivo
 *   ../archivo
 *   /
 */
static fs_node_t* fs_resolve_path(const char* path) {
    if (!path || path[0] == '\0') {
        return fs_current;
    }

    fs_node_t* node;

    if (path[0] == '/') {
        node = fs_root;
    } else {
        node = fs_current;
    }

    uint32_t i = 0;

    while (path[i]) {
        while (path[i] == '/') {
            i++;
        }

        if (path[i] == '\0') {
            break;
        }

        uint32_t start = i;

        while (path[i] && path[i] != '/') {
            i++;
        }

        uint32_t end = i;

        char part[32];
        fs_copy_part(part, path, start, end);

        if (part[0] == '\0' || fs_name_equals(part, ".")) {
            continue;
        }

        if (fs_name_equals(part, "..")) {
            if (node->parent) {
                node = node->parent;
            }

            continue;
        }

        if (node->type != FS_DIR) {
            return 0;
        }

        node = fs_find_child(node, part);

        if (!node) {
            return 0;
        }
    }

    return node;
}

/*
 * Para crear/eliminar cosas por ruta, separa la ruta en:
 *   padre + nombre final
 * Ejemplo:
 *   /home/docs/nota.txt -> padre=/home/docs, nombre=nota.txt
 */
static fs_node_t* fs_resolve_parent(const char* path, char* out_name) {
    if (!path || path[0] == '\0') {
        return 0;
    }

    uint32_t len = 0;

    while (path[len]) {
        len++;
    }

    /* Ignora barras finales: /home/docs/ -> /home/docs */
    while (len > 1 && path[len - 1] == '/') {
        len--;
    }

    uint32_t last_slash = len;
    uint32_t i = len;

    while (i > 0) {
        i--;

        if (path[i] == '/') {
            last_slash = i;
            break;
        }
    }

    /* No hay ninguna barra: el padre es el directorio actual. */
    if (last_slash == len) {
        fs_copy_part(out_name, path, 0, len);
        return fs_current;
    }

    fs_copy_part(out_name, path, last_slash + 1, len);

    if (out_name[0] == '\0') {
        return 0;
    }

    /* Caso /archivo: el padre es root. */
    if (last_slash == 0) {
        return fs_root;
    }

    char parent_path[256];

    for (uint32_t j = 0; j < 256; j++) {
        parent_path[j] = 0;
    }

    for (uint32_t j = 0; j < last_slash && j < 255; j++) {
        parent_path[j] = path[j];
    }

    parent_path[last_slash] = '\0';

    fs_node_t* parent = fs_resolve_path(parent_path);

    if (!parent || parent->type != FS_DIR) {
        return 0;
    }

    return parent;
}

static int fs_invalid_new_name(const char* name) {
    if (!name || name[0] == '\0') {
        return 1;
    }

    if (fs_name_equals(name, ".") || fs_name_equals(name, "..")) {
        return 1;
    }

    return 0;
}

void fs_init(void) {
    fs_root = fs_alloc_node();

    if (!fs_root) {
        kprintf("RAMFS: error al crear root\n");
        return;
    }

    fs_copy_name(fs_root->name, "/");
    fs_root->type = FS_DIR;
    fs_root->parent = fs_root;

    fs_current = fs_root;

    fs_mkdir("home");
    fs_mkdir("system");

//    kprintf("RAMFS iniciado correctamente\n");
}

void fs_print_current_path(void) {
    if (!fs_current || !fs_root) {
        kprintf("/");
        return;
    }

    if (fs_current == fs_root) {
        kprintf("/");
        return;
    }

    fs_node_t* stack[32];
    uint32_t count = 0;

    fs_node_t* node = fs_current;

    while (node && node != fs_root && count < 32) {
        stack[count++] = node;
        node = node->parent;
    }

    kprintf("/");

    for (int32_t i = count - 1; i >= 0; i--) {
        kprintf("%s", stack[i]->name);

        if (i != 0) {
            kprintf("/");
        }
    }
}

void fs_pwd(void) {
    fs_print_current_path();
    kprintf("\n");
}

int fs_mkdir(const char* path) {
    if (!fs_current || !path || path[0] == '\0') {
        return -1;
    }

    char name[32];
    fs_node_t* parent = fs_resolve_parent(path, name);

    if (!parent) {
        kprintf("Ruta invalida: %s\n", path);
        return -1;
    }

    if (fs_invalid_new_name(name)) {
        kprintf("Nombre invalido: %s\n", name);
        return -1;
    }

    if (fs_find_child(parent, name)) {
        kprintf("Ya existe: %s\n", name);
        return -1;
    }

    fs_node_t* node = fs_alloc_node();

    if (!node) {
        kprintf("Sin memoria para crear directorio\n");
        return -1;
    }

    fs_copy_name(node->name, name);
    node->type = FS_DIR;

    fs_add_child(parent, node);

    return 0;
}

int fs_mkdir_p(const char* path) {
    if (!fs_current || !fs_root || !path || path[0] == '\0') {
        return -1;
    }

    fs_node_t* current;

    if (path[0] == '/') {
        current = fs_root;
    } else {
        current = fs_current;
    }

    uint32_t i = 0;

    while (path[i]) {
        while (path[i] == '/') {
            i++;
        }

        if (path[i] == '\0') {
            break;
        }

        uint32_t start = i;

        while (path[i] && path[i] != '/') {
            i++;
        }

        uint32_t end = i;

        char part[32];

        for (uint32_t j = 0; j < 32; j++) {
            part[j] = 0;
        }

        fs_copy_part(part, path, start, end);

        if (part[0] == '\0') {
            continue;
        }

        if (part[0] == '.' && part[1] == '\0') {
            continue;
        }

        if (part[0] == '.' && part[1] == '.' && part[2] == '\0') {
            if (current->parent) {
                current = current->parent;
            }

            continue;
        }

        fs_node_t* child = fs_find_child(current, part);

        if (child) {
            if (child->type != FS_DIR) {
                kprintf("Existe pero no es directorio: %s\n", part);
                return -1;
            }

            current = child;
        } else {
            fs_node_t* node = fs_alloc_node();

            if (!node) {
                kprintf("Sin memoria para crear directorio\n");
                return -1;
            }

            fs_copy_name(node->name, part);
            node->type = FS_DIR;

            fs_add_child(current, node);

            current = node;
        }
    }

    return 0;
}

int fs_touch(const char* path) {
    if (!fs_current || !path || path[0] == '\0') {
        return -1;
    }

    char name[32];
    fs_node_t* parent = fs_resolve_parent(path, name);

    if (!parent) {
        kprintf("Ruta invalida: %s\n", path);
        return -1;
    }

    if (fs_invalid_new_name(name)) {
        kprintf("Nombre invalido: %s\n", name);
        return -1;
    }

    if (fs_find_child(parent, name)) {
        kprintf("Ya existe: %s\n", name);
        return -1;
    }

    fs_node_t* node = fs_alloc_node();

    if (!node) {
        kprintf("Sin memoria para crear archivo\n");
        return -1;
    }

    fs_copy_name(node->name, name);
    node->type = FS_FILE;
    node->size = 0;

    fs_add_child(parent, node);

    return 0;
}

int fs_cd(const char* path) {
    if (!fs_current || !path || path[0] == '\0') {
        return -1;
    }

    fs_node_t* node = fs_get_node(path);

    if (!node) {
        kprintf("No existe el directorio: %s\n", path);
        return -1;
    }

    if (node->type != FS_DIR) {
        kprintf("No es un directorio: %s\n", path);
        return -1;
    }

    fs_current = node;
    return 0;
}

int fs_write(const char* path, const char* text) {
    if (!fs_current || !path || !text) {
        return -1;
    }

    fs_node_t* node = fs_get_node(path);

    if (!node) {
        kprintf("No existe el archivo: %s\n", path);
        return -1;
    }

    if (node->type != FS_FILE) {
        kprintf("No es un archivo: %s\n", path);
        return -1;
    }

    uint32_t i = 0;

    while (text[i] && i < 511) {
        node->content[i] = text[i];
        i++;
    }

    node->content[i] = 0;
    node->size = i;

    if (text[i] != '\0') {
        kprintf("Aviso: texto cortado, limite de 511 caracteres\n");
    }

    return 0;
}

int fs_cat(const char* path) {
    if (!fs_current || !path || path[0] == '\0') {
        return -1;
    }

    fs_node_t* node = fs_get_node(path);

    if (!node) {
        kprintf("No existe el archivo: %s\n", path);
        return -1;
    }

    if (node->type != FS_FILE) {
        kprintf("No es un archivo: %s\n", path);
        return -1;
    }

    kprintf("%s\n", node->content);

    return 0;
}

/* 
static int fs_remove_child(fs_node_t* parent, fs_node_t* target) {
    if (!parent || !target) {
        return -1;
    }

    fs_node_t* current = parent->children;
    fs_node_t* previous = 0;

    while (current) {
        if (current == target) {
            if (previous) {
                previous->next = current->next;
            } else {
                parent->children = current->next;
            }

            current->next = 0;
            current->parent = 0;
            return 0;
        }

        previous = current;
        current = current->next;
    }

    return -1;
}
*/

static int fs_remove_child(fs_node_t* parent, fs_node_t* target) {
    if (!parent || !target) {
        return -1;
    }

    fs_node_t* current = parent->children;
    fs_node_t* previous = 0;

    while (current) {
        if (current == target) {
            if (previous) {
                previous->next = current->next;
            } else {
                parent->children = current->next;
            }

            target->next = 0;
            return 0;
        }

        previous = current;
        current = current->next;
    }

    return -1;
}

int fs_rm(const char* path) {
    if (!fs_current || !path || path[0] == '\0') {
        return -1;
    }

    fs_node_t* node = fs_get_node(path);

    if (!node) {
        kprintf("No existe: %s\n", path);
        return -1;
    }

    if (node == fs_root) {
        kprintf("No puedes borrar /\n");
        return -1;
    }

    if (node->type == FS_DIR && node->children) {
        kprintf("El directorio no esta vacio: %s\n", path);
        kprintf("Usa: rm -r %s\n", path);
        return -1;
    }

    if (fs_current == node) {
        fs_current = node->parent;
    }

    fs_remove_child(node->parent, node);

    kfree(node);

    return 0;
}

int fs_rmdir(const char* path) {
    return fs_rm(path);
}

int fs_append(const char* path, const char* text) {
    if (!fs_current || !path || !text) {
        return -1;
    }

    fs_node_t* node = fs_get_node(path);

    if (!node) {
        kprintf("No existe el archivo: %s\n", path);
        return -1;
    }

    if (node->type != FS_FILE) {
        kprintf("No es un archivo: %s\n", path);
        return -1;
    }

    uint32_t i = node->size;
    uint32_t j = 0;

    while (text[j] && i < 511) {
        node->content[i] = text[j];
        i++;
        j++;
    }

    node->content[i] = 0;
    node->size = i;

    if (text[j] != '\0') {
        kprintf("Aviso: texto cortado, limite de 511 caracteres\n");
    }

    return 0;
}

int fs_clearfile(const char* path) {
    if (!fs_current || !path || path[0] == '\0') {
        return -1;
    }

    fs_node_t* node = fs_get_node(path);

    if (!node) {
        kprintf("No existe el archivo: %s\n", path);
        return -1;
    }

    if (node->type != FS_FILE) {
        kprintf("No es un archivo: %s\n", path);
        return -1;
    }

    for (uint32_t i = 0; i < 512; i++) {
        node->content[i] = 0;
    }

    node->size = 0;

    return 0;
}

static void fs_tree_print_node(fs_node_t* node, uint32_t depth) {
    if (!node) {
        return;
    }

    for (uint32_t i = 0; i < depth; i++) {
        kprintf("  ");
    }

    if (node->type == FS_DIR) {
        kprintf("[DIR]  %s\n", node->name);
    } else {
        kprintf("[FILE] %s (%u bytes)\n", node->name, node->size);
    }

    if (node->type == FS_DIR) {
        fs_node_t* child = node->children;

        while (child) {
            fs_tree_print_node(child, depth + 1);
            child = child->next;
        }
    }
}

void fs_tree(const char* path) {
    if (!fs_root) {
        kprintf("RAMFS no iniciado\n");
        return;
    }

    fs_node_t* target;

    if (!path || path[0] == '\0') {
        target = fs_current;
    } else {
        target = fs_resolve_path(path);
    }

    if (!target) {
        kprintf("Ruta no encontrada: %s\n", path);
        return;
    }

    fs_tree_print_node(target, 0);
}

void fs_ls(const char* path) {
    fs_node_t* target;

    if (!path || path[0] == '\0') {
        target = fs_current;
    } else {
        target = fs_resolve_path(path);
    }

    if (!target) {
        kprintf("Ruta no encontrada: %s\n", path);
        return;
    }

    if (target->type == FS_FILE) {
        kprintf("[FILE] %s (%u bytes)\n", target->name, target->size);
        return;
    }

    fs_node_t* child = target->children;

    while (child) {
        if (child->type == FS_DIR) {
            kprintf("[DIR]  %s\n", child->name);
        } else {
            kprintf("[FILE] %s (%u bytes)\n", child->name, child->size);
        }

        child = child->next;
    }
}

int fs_rename(const char* path, const char* new_name) {
    if (!fs_current || !path || !new_name || path[0] == '\0' || new_name[0] == '\0') {
        return -1;
    }

    fs_node_t* node = fs_get_node(path);

    if (!node) {
        kprintf("No existe: %s\n", path);
        return -1;
    }

    if (node == fs_root) {
        kprintf("No puedes renombrar /\n");
        return -1;
    }

    if (new_name[0] == '.' && new_name[1] == '\0') {
        kprintf("Nombre invalido: .\n");
        return -1;
    }

    if (new_name[0] == '.' && new_name[1] == '.' && new_name[2] == '\0') {
        kprintf("Nombre invalido: ..\n");
        return -1;
    }

    uint32_t i = 0;
    while (new_name[i]) {
        if (new_name[i] == '/') {
            kprintf("El nuevo nombre no puede contener '/'\n");
            return -1;
        }

        i++;
    }

    if (i >= 32) {
        kprintf("Nombre demasiado largo, maximo 31 caracteres\n");
        return -1;
    }

    if (fs_find_child(node->parent, new_name)) {
        kprintf("Ya existe un elemento llamado: %s\n", new_name);
        return -1;
    }

    fs_copy_name(node->name, new_name);

    return 0;
}

static int fs_is_descendant(fs_node_t* possible_parent, fs_node_t* node) {
    fs_node_t* current = node;

    while (current) {
        if (current == possible_parent) {
            return 1;
        }

        if (current == fs_root) {
            break;
        }

        current = current->parent;
    }

    return 0;
}

int fs_mv(const char* src_path, const char* dst_path) {
    if (!fs_current || !src_path || !dst_path || src_path[0] == '\0' || dst_path[0] == '\0') {
        return -1;
    }

    fs_node_t* src = fs_resolve_path(src_path);

    if (!src) {
        kprintf("No existe el origen: %s\n", src_path);
        return -1;
    }

    if (src == fs_root) {
        kprintf("No puedes mover /\n");
        return -1;
    }

    fs_node_t* dst = fs_resolve_path(dst_path);

    fs_node_t* new_parent = 0;
    char new_name[32];

    for (uint32_t i = 0; i < 32; i++) {
        new_name[i] = 0;
    }

    if (dst && dst->type == FS_DIR) {
        new_parent = dst;
        fs_copy_name(new_name, src->name);
    } else if (dst && dst->type == FS_FILE) {
        kprintf("El destino ya existe y es un archivo: %s\n", dst_path);
        return -1;
    } else {
        new_parent = fs_resolve_parent(dst_path, new_name);

        if (!new_parent) {
            kprintf("Ruta destino invalida: %s\n", dst_path);
            return -1;
        }

        if (new_parent->type != FS_DIR) {
            kprintf("El padre destino no es un directorio\n");
            return -1;
        }
    }

    if (!new_parent || new_name[0] == '\0') {
        kprintf("Destino invalido\n");
        return -1;
    }

    if (new_name[0] == '.' && new_name[1] == '\0') {
        kprintf("Nombre destino invalido: .\n");
        return -1;
    }

    if (new_name[0] == '.' && new_name[1] == '.' && new_name[2] == '\0') {
        kprintf("Nombre destino invalido: ..\n");
        return -1;
    }

    uint32_t len = 0;
    while (new_name[len]) {
        if (new_name[len] == '/') {
            kprintf("Nombre destino invalido\n");
            return -1;
        }

        len++;
    }

    if (len >= 32) {
        kprintf("Nombre destino demasiado largo\n");
        return -1;
    }

    if (src->type == FS_DIR && fs_is_descendant(src, new_parent)) {
        kprintf("No puedes mover un directorio dentro de si mismo\n");
        return -1;
    }

    fs_node_t* existing = fs_find_child(new_parent, new_name);

    if (existing && existing != src) {
        kprintf("Ya existe en destino: %s\n", new_name);
        return -1;
    }

    fs_remove_child(src->parent, src);

    fs_copy_name(src->name, new_name);
    fs_add_child(new_parent, src);

    return 0;
}

int fs_cp(const char* src_path, const char* dst_path) {
    if (!fs_current || !src_path || !dst_path || src_path[0] == '\0' || dst_path[0] == '\0') {
        return -1;
    }

    fs_node_t* src = fs_resolve_path(src_path);

    if (!src) {
        kprintf("No existe el origen: %s\n", src_path);
        return -1;
    }

    if (src->type != FS_FILE) {
        kprintf("cp solo soporta archivos por ahora\n");
        return -1;
    }

    fs_node_t* dst = fs_resolve_path(dst_path);

    fs_node_t* parent = 0;
    char new_name[32];

    for (uint32_t i = 0; i < 32; i++) {
        new_name[i] = 0;
    }

    /*
       Caso 1:
       cp archivo.txt /home
       Si destino existe y es directorio, copiamos dentro con el mismo nombre.
    */
    if (dst && dst->type == FS_DIR) {
        parent = dst;
        fs_copy_name(new_name, src->name);
    }

    /*
       Caso 2:
       cp archivo.txt /home/copia.txt
       Si destino no existe, usamos el padre de la ruta y el nombre final.
    */
    else if (!dst) {
        parent = fs_resolve_parent(dst_path, new_name);

        if (!parent) {
            kprintf("Ruta destino invalida: %s\n", dst_path);
            return -1;
        }

        if (parent->type != FS_DIR) {
            kprintf("El padre destino no es un directorio\n");
            return -1;
        }
    }

    /*
       Caso 3:
       destino existe y es archivo.
       De momento no sobrescribimos para evitar bugs.
    */
    else {
        kprintf("El destino ya existe: %s\n", dst_path);
        return -1;
    }

    if (!parent || new_name[0] == '\0') {
        kprintf("Destino invalido\n");
        return -1;
    }

    if (new_name[0] == '.' && new_name[1] == '\0') {
        kprintf("Nombre destino invalido: .\n");
        return -1;
    }

    if (new_name[0] == '.' && new_name[1] == '.' && new_name[2] == '\0') {
        kprintf("Nombre destino invalido: ..\n");
        return -1;
    }

    uint32_t name_len = 0;

    while (new_name[name_len]) {
        if (new_name[name_len] == '/') {
            kprintf("Nombre destino invalido\n");
            return -1;
        }

        name_len++;
    }

    if (name_len >= 32) {
        kprintf("Nombre destino demasiado largo\n");
        return -1;
    }

    if (fs_find_child(parent, new_name)) {
        kprintf("Ya existe en destino: %s\n", new_name);
        return -1;
    }

    fs_node_t* copy = fs_alloc_node();

    if (!copy) {
        kprintf("Sin memoria para copiar archivo\n");
        return -1;
    }

    fs_copy_name(copy->name, new_name);
    copy->type = FS_FILE;
    copy->size = src->size;

    for (uint32_t i = 0; i < 512; i++) {
        copy->content[i] = src->content[i];
    }

    fs_add_child(parent, copy);

    return 0;
}

static void fs_free_recursive(fs_node_t* node) {
    if (!node) {
        return;
    }

    if (node->type == FS_DIR) {
        fs_node_t* child = node->children;

        while (child) {
            fs_node_t* next = child->next;
            fs_free_recursive(child);
            child = next;
        }

        node->children = 0;
    }

    kfree(node);
}

int fs_rm_recursive(const char* path) {
    if (!fs_current || !fs_root || !path || path[0] == '\0') {
        return -1;
    }

    fs_node_t* node = fs_get_node(path);

    if (!node) {
        kprintf("No existe: %s\n", path);
        return -1;
    }

    if (node == fs_root) {
        kprintf("No puedes borrar / con rm -r\n");
        return -1;
    }

    /*
       Si el directorio actual esta dentro de lo que vamos a borrar,
       subimos al padre del nodo borrado.
    */
    fs_node_t* check = fs_current;

    while (check && check != fs_root) {
        if (check == node) {
            fs_current = node->parent;
            break;
        }

        check = check->parent;
    }

    fs_remove_child(node->parent, node);
    fs_free_recursive(node);

    return 0;
}

static int fs_contains(const char* text, const char* query) {
    if (!text || !query || query[0] == '\0') {
        return 0;
    }

    for (uint32_t i = 0; text[i]; i++) {
        uint32_t j = 0;

        while (query[j] && text[i + j] && text[i + j] == query[j]) {
            j++;
        }

        if (query[j] == '\0') {
            return 1;
        }
    }

    return 0;
}

static void fs_print_node_path(fs_node_t* node) {
    if (!node || !fs_root) {
        kprintf("/");
        return;
    }

    if (node == fs_root) {
        kprintf("/");
        return;
    }

    fs_node_t* stack[32];
    uint32_t count = 0;

    fs_node_t* current = node;

    while (current && current != fs_root && count < 32) {
        stack[count++] = current;
        current = current->parent;
    }

    kprintf("/");

    for (int32_t i = count - 1; i >= 0; i--) {
        kprintf("%s", stack[i]->name);

        if (i != 0) {
            kprintf("/");
        }
    }
}

static void fs_find_recursive(fs_node_t* node, const char* query, uint32_t* found_count) {
    if (!node || !query) {
        return;
    }

    if (fs_contains(node->name, query)) {
        fs_print_node_path(node);

        if (node->type == FS_DIR) {
            kprintf(" [DIR]\n");
        } else {
            kprintf(" [FILE, %u bytes]\n", node->size);
        }

        (*found_count)++;
    }

    if (node->type == FS_DIR) {
        fs_node_t* child = node->children;

        while (child) {
            fs_find_recursive(child, query, found_count);
            child = child->next;
        }
    }
}

void fs_find(const char* start_path, const char* query) {
    if (!fs_root) {
        kprintf("RAMFS no iniciado\n");
        return;
    }

    if (!query || query[0] == '\0') {
        kprintf("Uso: find [ruta] texto\n");
        return;
    }

    fs_node_t* start;

    if (!start_path || start_path[0] == '\0') {
        start = fs_root;
    } else {
        start = fs_resolve_path(start_path);
    }

    if (!start) {
        kprintf("Ruta no encontrada: %s\n", start_path);
        return;
    }

    uint32_t found_count = 0;

    fs_find_recursive(start, query, &found_count);

    if (found_count == 0) {
        kprintf("No se encontraron resultados para: %s\n", query);
    }
}

fs_node_t* fs_get_root(void) {
    return fs_root;
}

fs_node_t* fs_get_current(void) {
    return fs_current;
}

void fs_set_current(fs_node_t* node) {
    if (!node) {
        return;
    }

    if (node->type != FS_DIR) {
        return;
    }

    fs_current = node;
}

uint32_t fs_count_children(fs_node_t* dir) {
    if (!dir || dir->type != FS_DIR) {
        return 0;
    }

    uint32_t count = 0;
    fs_node_t* child = dir->children;

    while (child) {
        count++;
        child = child->next;
    }

    return count;
}

fs_node_t* fs_get_child_at(fs_node_t* dir, uint32_t index) {
    if (!dir || dir->type != FS_DIR) {
        return 0;
    }

    fs_node_t* child = dir->children;
    uint32_t current = 0;

    while (child) {
        if (current == index) {
            return child;
        }

        current++;
        child = child->next;
    }

    return 0;
}

fs_node_t* fs_get_node(const char* path) {
    return fs_resolve_path(path);
}