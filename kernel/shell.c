#include "../include/shell.h"
#include "../include/vga.h"
#include "../include/kprintf.h"
#include "../include/pmm.h"
#include "../include/fs.h"
#include "../include/kmalloc.h"
#include "../include/gui.h"
#define SHELL_BUFFER_SIZE 256

static char    buffer[SHELL_BUFFER_SIZE];
static uint32_t buf_pos = 0;

/* Compara dos strings */
static int kstrcmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

/* Compara n caracteres */
static int kstrncmp(const char *a, const char *b, uint32_t n) {
    while (n-- && *a && *b && *a == *b) { a++; b++; }
    return n == (uint32_t)-1 ? 0 : *a - *b;
}

static uint32_t kstrlen(const char *s) {
    uint32_t i = 0;
    while (s[i]) i++;
    return i;
}

static void cmd_heap(void) {
    uint32_t used = kmalloc_used();
    uint32_t total = kmalloc_total();
    uint32_t free = total > used ? total - used : 0;

    kprintf_color(VGA_YELLOW, VGA_BLACK, "\n=== Heap kmalloc ===\n");
    kprintf("Total: %u bytes (%u KB)\n", total, total / 1024);
    kprintf("Usado: %u bytes (%u KB)\n", used, used / 1024);
    kprintf("Libre: %u bytes (%u KB)\n\n", free, free / 1024);
}

static void prompt(void) {
    kprintf_color(VGA_LIGHT_GREEN,  VGA_BLACK, "bionic");
    kprintf_color(VGA_LIGHT_GREY,   VGA_BLACK, "@");
    kprintf_color(VGA_LIGHT_CYAN,   VGA_BLACK, "kernel");
    kprintf_color(VGA_WHITE,   VGA_BLACK, ":");

    fs_print_current_path();

    kprintf_color(VGA_WHITE,   VGA_BLACK, "> ");
}

/* ── Comandos ── */

static void cmd_help(void) {
    kprintf_color(VGA_YELLOW, VGA_BLACK, "\nComandos disponibles:\n");
    kprintf_color(VGA_WHITE, VGA_BLACK, "  Uso general:\n");
    kprintf("    help                      muestra esta ayuda\n");
    kprintf("    clear                     limpia la pantalla\n");
    kprintf("    echo <texto>              repite el texto\n");
    kprintf("    info                      informacion del kernel\n");
    kprintf("    color                     muestra la paleta de colores\n");
    kprintf("    mem                       muestra el uso de memoria\n");
    kprintf_color(VGA_WHITE, VGA_BLACK,  "\n  RamFS (Sistema de archivos en memoria RAM, experimental):\n");
    kprintf("    ls [ruta]                 lista archivos y directorios\n");
    kprintf("    pwd                       muestra el directorio actual\n");    
    kprintf("    mkdir [-p] <ruta>         crea directorios\n");
    kprintf("    touch <nombre>            crea un nuevo archivo\n");
    kprintf("    cd <nombre>               cambia al directorio especificado\n");
    kprintf("    cat <nombre>              muestra el contenido de un archivo\n");
    kprintf("    write <archivo> <texto>   escribe texto en un archivo\n");
    kprintf("    append <archivo> <texto>  agrega texto al final de un archivo\n");
    kprintf("    clearfile <archivo>       borra el contenido de un archivo\n");
    kprintf("    rm <ruta>                 elimina archivos o carpetas vacias\n");
    kprintf("    rm -r <ruta>              elimina carpetas recursivamente\n");
    kprintf("    rmdir <ruta>              alias de rm para carpetas vacias\n");
    kprintf_color(VGA_WHITE, VGA_BLACK, "Para ver mas comandos, escribe 'help2'\n");
    kprintf("\n");
}

static void cmd_help2(void) {
    kprintf("    tree [ruta]               muestra la estructura de archivos\n");
    kprintf("    rename <ruta> <nombre>    renombra archivo o carpeta\n");
    kprintf("    mv <origen> <destino>     mueve un archivo o carpeta\n");
    kprintf("    cp <origen> <destino>     copia un archivo\n");
    kprintf("    find [ruta] <texto>       busca archivos o carpetas por nombre\n");
    kprintf("    heap                      muestra uso del heap kmalloc\n");
    kprintf("    gui                       inicia una demo grafica basica\n");
    kprintf("\n");
}

static void cmd_clear(void) {
    vga_clear();
}

static void cmd_echo(const char *arg) {
    if (!arg || *arg == '\0') {
        kprintf("\n");
        return;
    }
    kprintf("%s\n", arg);
}

static void cmd_info(void) {
    kprintf_color(VGA_YELLOW, VGA_BLACK, "\n=== Bionic Kernel 0.3.2 ===\n");
    kprintf("Arquitectura:  x86 32-bit\n");
    kprintf("Modo de video: texto 80x25\n");
    kprintf("Colores:       16\n");
    kprintf("GDT:           3 descriptores\n");
    kprintf("IDT:           256 entradas\n");
    kprintf("IRQs:          16 (remapeadas a INT 32-47)\n");
    kprintf("\n");
}

static void cmd_color(void) {
    kprintf("\n");
    const char *nombres[] = {
        "BLACK", "BLUE", "GREEN", "CYAN",
        "RED", "MAGENTA", "BROWN", "L.GREY",
        "D.GREY", "L.BLUE", "L.GREEN", "L.CYAN",
        "L.RED", "L.MAGENTA", "YELLOW", "WHITE"
    };
    for (int i = 0; i < 16; i++) {
        vga_puts_color(nombres[i], (vga_color_t)i, VGA_BLACK);
        kprintf(" ");
    }
    kprintf("\n\n");
}

static void cmd_unknown(const char *cmd) {
    kprintf_color(VGA_LIGHT_RED, VGA_BLACK, "Comando no encontrado: ");
    kprintf("%s\n", cmd);
    kprintf("Escribe 'help' para ver los comandos disponibles.\n");
}

static void cmd_mem(void) {
    uint32_t free  = pmm_free_frames()  * PAGE_SIZE / 1024;
    uint32_t total = pmm_total_frames() * PAGE_SIZE / 1024;
    uint32_t used  = total - free;

    kprintf_color(VGA_YELLOW, VGA_BLACK, "\n=== Memoria fisica ===\n");
    kprintf("Total:  %u KB (%u MB)\n", total, total / 1024);
    kprintf("Usada:  %u KB (%u MB)\n", used,  used  / 1024);
    kprintf("Libre:  %u KB (%u MB)\n", free,  free  / 1024);

    /* Barra visual de uso */
    kprintf("\n[");
    uint32_t bar_total = 40;
    uint32_t bar_used  = total ? (used * bar_total) / total : 0;
    for (uint32_t i = 0; i < bar_total; i++) {
        if (i < bar_used)
            vga_puts_color("#", VGA_LIGHT_RED,   VGA_BLACK);
        else
            vga_puts_color("-", VGA_LIGHT_GREEN, VGA_BLACK);
    }
    kprintf("] %u%%\n\n", total ? (used * 100) / total : 0);
}

static void execute(void) {
    /* Separa comando y argumento en el primer espacio */
    char *cmd = buffer;
    char *arg = (void*)0;

    for (uint32_t i = 0; i < buf_pos; i++) {
        if (buffer[i] == ' ') {
            buffer[i] = '\0';
            arg = &buffer[i + 1];
            break;
        }
    }

    if (buf_pos == 0) {
        /* Enter en vacío, solo muestra prompt */
    } else if (kstrcmp(cmd, "help")  == 0) { cmd_help();     }
    else if (kstrcmp(cmd, "help2") == 0) { cmd_help2();      }
    else if (kstrcmp(cmd, "clear") == 0) { cmd_clear();      }
    else if (kstrcmp(cmd, "info")  == 0) { cmd_info();       }
    else if (kstrcmp(cmd, "color") == 0) { cmd_color();      }
    else if (kstrcmp(cmd, "echo")  == 0) { cmd_echo(arg ? arg : ""); }
    else if (kstrcmp(cmd, "mem")   == 0) { cmd_mem();        }
else if (kstrcmp(cmd, "ls") == 0) {
    fs_ls(arg ? arg : "");
}
else if (kstrcmp(cmd, "pwd") == 0) {
    fs_pwd();
}
else if (kstrcmp(cmd, "mkdir") == 0) {
    if (!arg || arg[0] == '\0') {
        kprintf("Uso: mkdir [-p] ruta\n");
    } else {
        if (arg[0] == '-' && arg[1] == 'p' && arg[2] == ' ') {
            const char* path = arg + 3;

            if (path[0] == '\0') {
                kprintf("Uso: mkdir -p ruta\n");
            } else {
                fs_mkdir_p(path);
            }
        } else {
            fs_mkdir(arg);
        }
    }
}
else if (kstrcmp(cmd, "touch") == 0) {
    if (!arg || arg[0] == '\0') {
        kprintf("Uso: touch archivo\n");
    } else {
        fs_touch(arg);
    }
}
else if (kstrcmp(cmd, "cd") == 0) {
    if (!arg || arg[0] == '\0') {
        kprintf("Uso: cd directorio\n");
    } else {
        fs_cd(arg);
    }
}
else if (kstrcmp(cmd, "cat") == 0) {
    if (!arg || arg[0] == '\0') {
        kprintf("Uso: cat archivo\n");
    } else {
        fs_cat(arg);
    }
}
else if (kstrcmp(cmd, "write") == 0) {
    if (!arg || arg[0] == '\0') {
        kprintf("Uso: write archivo texto\n");
    } else {
        uint32_t i = 0;

        while (arg[i] && arg[i] != ' ') {
            i++;
        }

        if (arg[i] == '\0') {
            kprintf("Uso: write archivo texto\n");
        } else {
            char filename[32];

            for (uint32_t j = 0; j < 32; j++) {
                filename[j] = 0;
            }

            for (uint32_t j = 0; j < i && j < 31; j++) {
                filename[j] = arg[j];
            }

            const char* text = arg + i + 1;

            fs_write(filename, text);
        }
    }
}
else if (kstrcmp(cmd, "rm") == 0) {
    if (!arg || arg[0] == '\0') {
        kprintf("Uso: rm [-r] ruta\n");
    } else {
        if (arg[0] == '-' && arg[1] == 'r' && arg[2] == ' ') {
            const char* path = arg + 3;

            if (path[0] == '\0') {
                kprintf("Uso: rm -r ruta\n");
            } else {
                fs_rm_recursive(path);
            }
        } else {
            fs_rm(arg);
        }
    }
}
else if (kstrcmp(cmd, "rmdir") == 0) {
    if (!arg || arg[0] == '\0') {
        kprintf("Uso: rmdir carpeta\n");
    } else {
        fs_rmdir(arg);
    }
}
else if (kstrcmp(cmd, "append") == 0) {
    if (!arg || arg[0] == '\0') {
        kprintf("Uso: append archivo texto\n");
    } else {
        uint32_t i = 0;

        while (arg[i] && arg[i] != ' ') {
            i++;
        }

        if (arg[i] == '\0') {
            kprintf("Uso: append archivo texto\n");
        } else {
            char filename[32];

            for (uint32_t j = 0; j < 32; j++) {
                filename[j] = 0;
            }

            for (uint32_t j = 0; j < i && j < 31; j++) {
                filename[j] = arg[j];
            }

            const char* text = arg + i + 1;

            fs_append(filename, text);
        }
    }
}
else if (kstrcmp(cmd, "clearfile") == 0) {
    if (!arg || arg[0] == '\0') {
        kprintf("Uso: clearfile archivo\n");
    } else {
        fs_clearfile(arg);
    }
}
else if (kstrcmp(cmd, "tree") == 0) {
    fs_tree(arg ? arg : "");
}
else if (kstrcmp(cmd, "rename") == 0) {
    if (!arg || arg[0] == '\0') {
        kprintf("Uso: rename ruta nuevo_nombre\n");
    } else {
        uint32_t i = 0;

        while (arg[i] && arg[i] != ' ') {
            i++;
        }

        if (arg[i] == '\0') {
            kprintf("Uso: rename ruta nuevo_nombre\n");
        } else {
            char old_path[128];
            char new_name[32];

            for (uint32_t j = 0; j < 128; j++) {
                old_path[j] = 0;
            }

            for (uint32_t j = 0; j < 32; j++) {
                new_name[j] = 0;
            }

            for (uint32_t j = 0; j < i && j < 127; j++) {
                old_path[j] = arg[j];
            }

            const char* name_start = arg + i + 1;

            uint32_t k = 0;
            while (name_start[k] && k < 31) {
                new_name[k] = name_start[k];
                k++;
            }

            new_name[k] = 0;

            fs_rename(old_path, new_name);
        }
    }
}

else if (kstrcmp(cmd, "mv") == 0) {
    if (!arg || arg[0] == '\0') {
        kprintf("Uso: mv origen destino\n");
    } else {
        uint32_t i = 0;

        while (arg[i] && arg[i] != ' ') {
            i++;
        }

        if (arg[i] == '\0') {
            kprintf("Uso: mv origen destino\n");
        } else {
            char src[128];
            char dst[128];

            for (uint32_t j = 0; j < 128; j++) {
                src[j] = 0;
                dst[j] = 0;
            }

            for (uint32_t j = 0; j < i && j < 127; j++) {
                src[j] = arg[j];
            }

            const char* dst_start = arg + i + 1;

            uint32_t k = 0;
            while (dst_start[k] && k < 127) {
                dst[k] = dst_start[k];
                k++;
            }

            dst[k] = 0;

            fs_mv(src, dst);
        }
    }
}

else if (kstrcmp(cmd, "cp") == 0) {
    if (!arg || arg[0] == '\0') {
        kprintf("Uso: cp origen destino\n");
    } else {
        uint32_t i = 0;

        while (arg[i] && arg[i] != ' ') {
            i++;
        }

        if (arg[i] == '\0') {
            kprintf("Uso: cp origen destino\n");
        } else {
            char src[128];
            char dst[128];

            for (uint32_t j = 0; j < 128; j++) {
                src[j] = 0;
                dst[j] = 0;
            }

            for (uint32_t j = 0; j < i && j < 127; j++) {
                src[j] = arg[j];
            }

            const char* dst_start = arg + i + 1;

            uint32_t k = 0;
            while (dst_start[k] && k < 127) {
                dst[k] = dst_start[k];
                k++;
            }

            dst[k] = 0;

            fs_cp(src, dst);
        }
    }
}

else if (kstrcmp(cmd, "find") == 0) {
    if (!arg || arg[0] == '\0') {
        kprintf("Uso: find [ruta] texto\n");
    } else {
        uint32_t i = 0;

        while (arg[i] && arg[i] != ' ') {
            i++;
        }

        /*
           Si no hay espacio:
           find nota
           Busca "nota" desde /
        */
        if (arg[i] == '\0') {
            fs_find("/", arg);
        } else {
            /*
               Si hay espacio:
               find /home nota
               Busca "nota" dentro de /home
            */
            char path[128];
            char query[64];

            for (uint32_t j = 0; j < 128; j++) {
                path[j] = 0;
            }

            for (uint32_t j = 0; j < 64; j++) {
                query[j] = 0;
            }

            for (uint32_t j = 0; j < i && j < 127; j++) {
                path[j] = arg[j];
            }

            const char* query_start = arg + i + 1;

            uint32_t k = 0;
            while (query_start[k] && k < 63) {
                query[k] = query_start[k];
                k++;
            }

            query[k] = 0;

            fs_find(path, query);
        }
    }
}

else if (kstrcmp(cmd, "gui") == 0) {
    gui_demo();
}

else if (kstrcmp(cmd, "heap") == 0) {
    cmd_heap();
}

    else  { cmd_unknown(cmd); }

    /* Limpia el buffer */
    buf_pos = 0;
    buffer[0] = '\0';
}

/* ── API pública ── */

void shell_init(void) {
    kprintf("Escribe 'help' para ver los comandos.\n\n");
    prompt();
}

void shell_handle_key(char c) {
    if (c == '\n') {
        kprintf("\n");
        execute();
        prompt();
        return;
    }

    if (c == '\b') {
        if (buf_pos > 0) {
            buf_pos--;
            buffer[buf_pos] = '\0';
            vga_putchar('\b');
        }
        return;
    }

    if (buf_pos < SHELL_BUFFER_SIZE - 1) {
        buffer[buf_pos++] = c;
        buffer[buf_pos]   = '\0';
        vga_putchar(c);
    }
}