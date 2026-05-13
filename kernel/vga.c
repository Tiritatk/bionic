#include "../include/vga.h"

#define VGA_WIDTH    80
#define VGA_HEIGHT   25
#define VGA_BUFFER   ((uint16_t *)0xB8000)

/* Estado interno del terminal */
static uint8_t  vga_row;
static uint8_t  vga_col;
static uint8_t  vga_attr;   /* color actual empaquetado */
static uint16_t *vga_buf;

/* Empaqueta fg+bg en el byte de atributo VGA */
static inline uint8_t make_color(vga_color_t fg, vga_color_t bg) {
    return fg | (bg << 4);
}

/* Empaqueta carácter + atributo en una celda de 16 bits */
static inline uint16_t make_entry(char c, uint8_t attr) {
    return (uint16_t)c | ((uint16_t)attr << 8);
}

void vga_init(void) {
    vga_row  = 0;
    vga_col  = 0;
    vga_attr = make_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_buf  = VGA_BUFFER;
    vga_clear();
}

void vga_set_color(vga_color_t fg, vga_color_t bg) {
    vga_attr = make_color(fg, bg);
}

void vga_clear(void) {
    uint16_t blank = make_entry(' ', vga_attr);
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga_buf[i] = blank;
    vga_row = 0;
    vga_col = 0;
}

/* Desplaza todas las líneas una posición hacia arriba */
static void vga_scroll(void) {
    /* Copia líneas 1..24 → 0..23 */
    for (int row = 0; row < VGA_HEIGHT - 1; row++)
        for (int col = 0; col < VGA_WIDTH; col++)
            vga_buf[row * VGA_WIDTH + col] =
                vga_buf[(row + 1) * VGA_WIDTH + col];

    /* Borra la última línea */
    uint16_t blank = make_entry(' ', vga_attr);
    for (int col = 0; col < VGA_WIDTH; col++)
        vga_buf[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = blank;

    vga_row = VGA_HEIGHT - 1;
}

void vga_set_cursor(uint8_t row, uint8_t col) {
    vga_row = row;
    vga_col = col;
}

void vga_putchar(char c) {
    if (c == '\b') {
        if (vga_col > 0) {
            vga_col--;
        } else if (vga_row > 0) {
            vga_row--;
        vga_col = VGA_WIDTH - 1;
    }
    vga_buf[vga_row * VGA_WIDTH + vga_col] = make_entry(' ', vga_attr);
    return;
    }

    if (c == '\n') {
        vga_col = 0;
        if (++vga_row == VGA_HEIGHT)
            vga_scroll();
        return;
    }

    if (c == '\r') {
        vga_col = 0;
        return;
    }

    if (c == '\t') {
        /* Tabulador: avanza al siguiente múltiplo de 8 */
        vga_col = (vga_col + 8) & ~7;
        if (vga_col >= VGA_WIDTH) {
            vga_col = 0;
            if (++vga_row == VGA_HEIGHT)
                vga_scroll();
        }
        return;
    }
    vga_buf[vga_row * VGA_WIDTH + vga_col] = make_entry(c, vga_attr);

    if (++vga_col == VGA_WIDTH) {
        vga_col = 0;
        if (++vga_row == VGA_HEIGHT)
            vga_scroll();
    }
}

void vga_puts(const char *str) {
    for (; *str; str++)
        vga_putchar(*str);
}

void vga_puts_color(const char *str, vga_color_t fg, vga_color_t bg) {
    uint8_t saved = vga_attr;
    vga_set_color(fg, bg);
    vga_puts(str);
    vga_attr = saved;   /* restaura el color anterior */
}