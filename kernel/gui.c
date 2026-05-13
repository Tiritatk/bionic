#include "../include/gui.h"
#include "../include/wallpaper.h"
#include "../include/kprintf.h"
#include "../include/fs.h"
#include "../include/io.h"
#include "../include/input.h"

#define VGA_WIDTH  gui_width
#define VGA_HEIGHT gui_height
#define GUI_COLOR_BG        1
#define GUI_COLOR_TOPBAR    8
#define GUI_COLOR_WINDOW    7
#define GUI_COLOR_TITLEBAR  9
#define GUI_COLOR_BORDER    15
#define GUI_COLOR_TEXT      0
#define GUI_COLOR_TEXT_INV  15
#define GUI_COLOR_BUTTON    10
#define GUI_COLOR_BUTTON_2  11
#define GUI_COLOR_STATUS    8
#define GUI_MENU_ITEMS      3
#define GUI_CONTEXT_ITEMS   6
#define DESKTOP_ICON_COUNT  3
#define GUI_TERM_MAX_LINES 32
#define GUI_TERM_LINE_LEN  96
#define GUI_TERM_INPUT_LEN 96
#define GUI_CURSOR_W       16
#define GUI_CURSOR_H       16
#define GUI_DOUBLE_CLICK_TSC_LIMIT 1000000000ULL

static void gui_draw_context_menu(void);
static void gui_open_context_menu(void);
static void gui_context_menu_handle_key(char c);
static void gui_open_new_file_dialog(void);
static void gui_draw_new_folder_dialog(void);
static void gui_open_new_folder_dialog(void);
static void gui_new_folder_handle_key(char c);
static void gui_delete_selected_item(void);
static void gui_restore_desktop_region(int x, int y, int w, int h);
static void gui_desktop_open_selected_app(void);
static void gui_desktop_open_selected_app(void);
static void gui_mouse_left_pressed(int32_t x, int32_t y);
static void gui_mouse_left_released(int32_t x, int32_t y);
static void gui_move_active_window(int dx, int dy);
static void gui_save_cursor_background(int32_t x, int32_t y);
static void gui_restore_cursor_background(int32_t x, int32_t y);
static void gui_mouse_process_buttons_and_drag(int32_t x, int32_t y, uint8_t left, uint8_t right);
static void gui_open_context_menu(void);

static int32_t gui_mouse_x = 100;
static int32_t gui_mouse_y = 100;
static int32_t gui_mouse_old_x = 100;
static int32_t gui_mouse_old_y = 100;
static int32_t gui_drag_mouse_start_x = 0;
static int32_t gui_drag_mouse_start_y = 0;
static int32_t gui_drag_window_start_x = 0;
static int32_t gui_drag_window_start_y = 0;
static int32_t gui_drag_outline_x = 0;
static int32_t gui_drag_outline_y = 0;
static int32_t gui_last_click_x = 0;
static int32_t gui_last_click_y = 0;
static int32_t gui_last_click_item = -1;

static uint32_t desktop_selected_icon = 0;
static uint32_t gui_width = 320;
static uint32_t gui_height = 200;
static uint32_t gui_pitch = 320;
static uint32_t gui_bpp = 8;
static uint32_t gui_term_input_pos = 0;
static uint32_t gui_term_line_count = 0;
static uint32_t gui_cursor_backup[GUI_CURSOR_W * GUI_CURSOR_H];

static uint64_t gui_last_click_time = 0;

static uint8_t* gui_fb_addr = (uint8_t*)0xA0000;
static uint8_t gui_using_framebuffer = 0;
static uint8_t gui_mouse_right_prev = 0;
static uint8_t gui_dragging_window = 0;
static uint8_t gui_mouse_left_prev = 0;
static uint8_t gui_cursor_backup_valid = 0;
static uint8_t gui_drag_outline_visible = 0;

static char gui_term_input[GUI_TERM_INPUT_LEN];
static char gui_term_lines[GUI_TERM_MAX_LINES][GUI_TERM_LINE_LEN];

static int gui_point_in_rect(int32_t px, int32_t py, int32_t x, int32_t y, int32_t w, int32_t h);
static int gui_desktop_icon_at(int32_t x, int32_t y);


static int gui_point_in_rect(int32_t px, int32_t py, int32_t x, int32_t y, int32_t w, int32_t h) {
    return px >= x && px < x + w && py >= y && py < y + h;
}

static int gui_desktop_icon_at(int32_t x, int32_t y) {
    if (gui_point_in_rect(x, y, 50, 80, 140, 80)) {
        return 0; // Files
    }

    if (gui_point_in_rect(x, y, 50, 180, 140, 80)) {
        return 1; // Terminal
    }

    if (gui_point_in_rect(x, y, 50, 280, 140, 80)) {
        return 2; // Info
    }

    return -1;
}

static void gui_draw_cursor(int32_t x, int32_t y) {
    /*
       Cursor simple tipo flecha retro.
    */

    gui_putpixel(x, y, 15);
    gui_putpixel(x, y + 1, 15);
    gui_putpixel(x, y + 2, 15);
    gui_putpixel(x, y + 3, 15);
    gui_putpixel(x, y + 4, 15);
    gui_putpixel(x, y + 5, 15);
    gui_putpixel(x, y + 6, 15);
    gui_putpixel(x, y + 7, 15);
    gui_putpixel(x, y + 8, 15);
    gui_putpixel(x, y + 9, 15);
    gui_putpixel(x, y + 10, 15);
    gui_putpixel(x, y + 11, 15);

    gui_putpixel(x + 1, y + 1, 15);
    gui_putpixel(x + 1, y + 2, 15);
    gui_putpixel(x + 1, y + 3, 15);
    gui_putpixel(x + 1, y + 4, 15);
    gui_putpixel(x + 1, y + 5, 15);
    gui_putpixel(x + 1, y + 6, 15);
    gui_putpixel(x + 1, y + 7, 15);
    gui_putpixel(x + 1, y + 8, 15);

    gui_putpixel(x + 2, y + 2, 15);
    gui_putpixel(x + 2, y + 3, 15);
    gui_putpixel(x + 2, y + 4, 15);
    gui_putpixel(x + 2, y + 5, 15);
    gui_putpixel(x + 2, y + 6, 15);

    gui_putpixel(x + 3, y + 3, 15);
    gui_putpixel(x + 3, y + 4, 15);
    gui_putpixel(x + 3, y + 5, 15);

    gui_putpixel(x + 4, y + 4, 15);
    gui_putpixel(x + 5, y + 5, 15);
    gui_putpixel(x + 6, y + 6, 15);

    /*
       Borde negro para que se vea mejor.
    */
    gui_putpixel(x + 1, y, 0);
    gui_putpixel(x + 2, y + 1, 0);
    gui_putpixel(x + 3, y + 2, 0);
    gui_putpixel(x + 4, y + 3, 0);
    gui_putpixel(x + 5, y + 4, 0);
    gui_putpixel(x + 6, y + 5, 0);
    gui_putpixel(x + 7, y + 6, 0);
}

void gui_mouse_moved(int32_t x, int32_t y, uint8_t left, uint8_t right) {
    /*
       1. Restaurar exactamente lo que había debajo del cursor anterior.
    */
    gui_restore_cursor_background(gui_mouse_old_x, gui_mouse_old_y);

    /*
       2. Procesar click/drag.
       Esto está en otra función porque active_window está definido más abajo.
    */
    gui_mouse_process_buttons_and_drag(x, y, left, right);

    /*
       3. Actualizar posición del cursor.
    */
    gui_mouse_x = x;
    gui_mouse_y = y;

    gui_mouse_left_prev = left;
    gui_mouse_right_prev = right;

    /*
       4. Guardar fondo bajo el cursor nuevo y dibujarlo.
    */
    gui_save_cursor_background(gui_mouse_x, gui_mouse_y);
    gui_draw_cursor(gui_mouse_x, gui_mouse_y);

    gui_mouse_old_x = gui_mouse_x;
    gui_mouse_old_y = gui_mouse_y;

    (void)right;
}

static const uint8_t g_320x200x256[] = {
    /* MISC */
    0x63,

    /* SEQ */
    0x03, 0x01, 0x0F, 0x00, 0x0E,

    /* CRTC */
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
    0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
    0xFF,

    /* GC */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
    0xFF,

    /* AC */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x41, 0x00, 0x0F, 0x00, 0x00
};

static void write_regs(const uint8_t* regs) {
    uint32_t i = 0;

    /* MISC */
    outb(0x3C2, regs[i]);
    i++;

    /* SEQUENCER */
    for (uint8_t index = 0; index < 5; index++) {
        outb(0x3C4, index);
        outb(0x3C5, regs[i]);
        i++;
    }

    /* Unlock CRTC registers */
    outb(0x3D4, 0x03);
    outb(0x3D5, inb(0x3D5) | 0x80);

    outb(0x3D4, 0x11);
    outb(0x3D5, inb(0x3D5) & ~0x80);

    /* CRTC */
    for (uint8_t index = 0; index < 25; index++) {
        outb(0x3D4, index);
        outb(0x3D5, regs[i]);
        i++;
    }

    /* GRAPHICS CONTROLLER */
    for (uint8_t index = 0; index < 9; index++) {
        outb(0x3CE, index);
        outb(0x3CF, regs[i]);
        i++;
    }

    /* ATTRIBUTE CONTROLLER */
    for (uint8_t index = 0; index < 21; index++) {
        inb(0x3DA);
        outb(0x3C0, index);
        outb(0x3C0, regs[i]);
        i++;
    }

    /* Enable display */
    inb(0x3DA);
    outb(0x3C0, 0x20);
}

static void gui_set_mode13h(void) {
    write_regs(g_320x200x256);
}

static uint32_t gui_palette_to_rgb(uint8_t color) {
    switch (color) {
        case 0:  return 0xFF000000; /* black */
        case 1:  return 0xFF0B1B8F; /* blue */
        case 2:  return 0xFF008000;
        case 3:  return 0xFF008080;
        case 4:  return 0xFF800000;
        case 5:  return 0xFF800080;
        case 6:  return 0xFF8A7F00;
        case 7:  return 0xFFB8B8B8;
        case 8:  return 0xFF111827;
        case 9:  return 0xFF1D4ED8;
        case 10: return 0xFF10B981;
        case 11: return 0xFF38BDF8;
        case 12: return 0xFFEF4444;
        case 13: return 0xFFD946EF;
        case 14: return 0xFFEAB308;
        case 15: return 0xFFFFFFFF;
        default: return 0xFF000000;
    }
}

void gui_init_from_multiboot(multiboot_info_t* mb) {
    if (!mb) {
        return;
    }

    if (!(mb->flags & MULTIBOOT_INFO_FRAMEBUFFER)) {
        return;
    }

    /* framebuffer_type 1 = RGB direct color. Es lo normal con gfxpayload=...x32 */
    if (mb->framebuffer_type != 1) {
        return;
    }

    if (mb->framebuffer_bpp != 32 && mb->framebuffer_bpp != 24) {
        return;
    }

    gui_fb_addr = (uint8_t*)(uint32_t)mb->framebuffer_addr;
    gui_width = mb->framebuffer_width;
    gui_height = mb->framebuffer_height;
    gui_pitch = mb->framebuffer_pitch;
    gui_bpp = mb->framebuffer_bpp;
    gui_using_framebuffer = 1;
}

int gui_has_framebuffer(void) {
    return gui_using_framebuffer != 0;
}

void gui_putpixel(uint32_t x, uint32_t y, uint8_t color) {
    if (!gui_fb_addr || x >= VGA_WIDTH || y >= VGA_HEIGHT) {
        return;
    }

    if (gui_bpp == 32) {
        uint32_t* pixel = (uint32_t*)(gui_fb_addr + y * gui_pitch + x * 4);
        *pixel = gui_palette_to_rgb(color);
        return;
    }

    if (gui_bpp == 24) {
        uint32_t rgb = gui_palette_to_rgb(color);
        uint8_t* pixel = gui_fb_addr + y * gui_pitch + x * 3;
        pixel[0] = (uint8_t)(rgb & 0xFF);
        pixel[1] = (uint8_t)((rgb >> 8) & 0xFF);
        pixel[2] = (uint8_t)((rgb >> 16) & 0xFF);
        return;
    }

    /* Fallback VGA 13h: color es indice de paleta */
    gui_fb_addr[y * gui_pitch + x] = color;
}


static void gui_putpixel_rgb(uint32_t x, uint32_t y, uint32_t rgb) {
    if (!gui_fb_addr || x >= VGA_WIDTH || y >= VGA_HEIGHT) {
        return;
    }

    if (gui_bpp == 32) {
        uint32_t* pixel = (uint32_t*)(gui_fb_addr + y * gui_pitch + x * 4);
        *pixel = rgb;
        return;
    }

    if (gui_bpp == 24) {
        uint8_t* pixel = gui_fb_addr + y * gui_pitch + x * 3;
        pixel[0] = (uint8_t)(rgb & 0xFF);
        pixel[1] = (uint8_t)((rgb >> 8) & 0xFF);
        pixel[2] = (uint8_t)((rgb >> 16) & 0xFF);
        return;
    }

    /* Fallback 8-bit: aproximacion simple a paleta VGA. */
    uint8_t r = (uint8_t)((rgb >> 16) & 0xFF);
    uint8_t g = (uint8_t)((rgb >> 8) & 0xFF);
    uint8_t b = (uint8_t)(rgb & 0xFF);

    if (r > 220 && g > 220 && b > 220) {
        gui_putpixel(x, y, 15);
    } else if (b > r && b > g) {
        gui_putpixel(x, y, 1);
    } else if (g > r && g > b) {
        gui_putpixel(x, y, 2);
    } else if (r > g && r > b) {
        gui_putpixel(x, y, 4);
    } else {
        gui_putpixel(x, y, 8);
    }
}

void gui_clear(uint8_t color) {
    for (uint32_t y = 0; y < VGA_HEIGHT; y++) {
        for (uint32_t x = 0; x < VGA_WIDTH; x++) {
            gui_putpixel(x, y, color);
        }
    }
}

void gui_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t color) {
    for (uint32_t yy = 0; yy < h; yy++) {
        for (uint32_t xx = 0; xx < w; xx++) {
            gui_putpixel(x + xx, y + yy, color);
        }
    }
}

void gui_rect_border(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t color) {
    for (uint32_t xx = 0; xx < w; xx++) {
        gui_putpixel(x + xx, y, color);
        gui_putpixel(x + xx, y + h - 1, color);
    }

    for (uint32_t yy = 0; yy < h; yy++) {
        gui_putpixel(x, y + yy, color);
        gui_putpixel(x + w - 1, y + yy, color);
    }
}

static const uint8_t font8x8_basic[128][8] = {
    [' '] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['!'] = {0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00},
    ['.'] = {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},
    [','] = {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30},
    [':'] = {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00},
    ['-'] = {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},
    ['_'] = {0x00,0x00,0x00,0x00,0x00,0x00,0x7E,0x00},
    ['/'] = {0x06,0x0C,0x18,0x30,0x60,0x00,0x00,0x00},
    ['>'] = {0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00},
    ['<'] = {0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00},
    ['['] = {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00},
    [']'] = {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00},

    ['0'] = {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00},
    ['1'] = {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},
    ['2'] = {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00},
    ['3'] = {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00},
    ['4'] = {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00},
    ['5'] = {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00},
    ['6'] = {0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0x00},
    ['7'] = {0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00},
    ['8'] = {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00},
    ['9'] = {0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00},

    ['A'] = {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00},
    ['B'] = {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00},
    ['C'] = {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00},
    ['D'] = {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00},
    ['E'] = {0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00},
    ['F'] = {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00},
    ['G'] = {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00},
    ['H'] = {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00},
    ['I'] = {0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00},
    ['J'] = {0x1E,0x0C,0x0C,0x0C,0x0C,0x6C,0x38,0x00},
    ['K'] = {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00},
    ['L'] = {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00},
    ['M'] = {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00},
    ['N'] = {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00},
    ['O'] = {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    ['P'] = {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00},
    ['Q'] = {0x3C,0x66,0x66,0x66,0x6E,0x3C,0x0E,0x00},
    ['R'] = {0x7C,0x66,0x66,0x7C,0x78,0x6C,0x66,0x00},
    ['S'] = {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00},
    ['T'] = {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
    ['U'] = {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    ['V'] = {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00},
    ['W'] = {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},
    ['X'] = {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00},
    ['Y'] = {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00},
    ['Z'] = {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00},
};

static char gui_to_upper(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 32;
    }

    return c;
}

void gui_draw_char(uint32_t x, uint32_t y, char c, uint8_t color) {
    c = gui_to_upper(c);

    if ((uint8_t)c >= 128) {
        c = '?';
    }

    const uint8_t* glyph = font8x8_basic[(uint8_t)c];

    for (uint32_t row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];

        for (uint32_t col = 0; col < 8; col++) {
            if (bits & (1 << (7 - col))) {
                gui_putpixel(x + col, y + row, color);
            }
        }
    }
}

void gui_draw_text(uint32_t x, uint32_t y, const char* text, uint8_t color) {
    uint32_t cursor_x = x;
    uint32_t cursor_y = y;

    for (uint32_t i = 0; text[i]; i++) {
        if (text[i] == '\n') {
            cursor_x = x;
            cursor_y += 10;
            continue;
        }

        gui_draw_char(cursor_x, cursor_y, text[i], color);
        cursor_x += 8;
    }
}

static uint32_t gui_text_width(const char* text) {
    uint32_t len = 0;

    if (!text) {
        return 0;
    }

    while (text[len]) {
        len++;
    }

    return len * 8;
}

static void gui_draw_wrapped_text(uint32_t x, uint32_t y, uint32_t max_chars_per_line, uint32_t max_lines, const char* text, uint8_t color) {
    if (!text) {
        return;
    }

    uint32_t cursor_x = x;
    uint32_t cursor_y = y;
    uint32_t line_chars = 0;
    uint32_t lines = 0;

    for (uint32_t i = 0; text[i] && lines < max_lines; i++) {
        if (text[i] == '\n') {
            cursor_x = x;
            cursor_y += 10;
            line_chars = 0;
            lines++;
            continue;
        }

        if (line_chars >= max_chars_per_line) {
            cursor_x = x;
            cursor_y += 10;
            line_chars = 0;
            lines++;
            if (lines >= max_lines) {
                break;
            }
        }

        gui_draw_char(cursor_x, cursor_y, text[i], color);
        cursor_x += 8;
        line_chars++;
    }
}

void gui_label(uint32_t x, uint32_t y, const char* text, uint8_t color) {
    gui_draw_text(x, y, text, color);
}

void gui_panel(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t bg, uint8_t border) {
    gui_rect(x, y, w, h, bg);
    gui_rect_border(x, y, w, h, border);
}

void gui_titlebar(uint32_t x, uint32_t y, uint32_t w, const char* title) {
    gui_rect(x, y, w, 18, GUI_COLOR_TITLEBAR);
    gui_draw_text(x + 8, y + 5, title, GUI_COLOR_TEXT_INV);

    /* Botones falsos de ventana */
    gui_rect(x + w - 36, y + 5, 8, 8, 12);
    gui_rect(x + w - 24, y + 5, 8, 8, 14);
    gui_rect(x + w - 12, y + 5, 8, 8, 10);
}

void gui_window(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const char* title) {
    gui_panel(x, y, w, h, GUI_COLOR_WINDOW, GUI_COLOR_BORDER);
    gui_titlebar(x, y, w, title);
}

void gui_button(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const char* text) {
    gui_rect(x, y, w, h, GUI_COLOR_BUTTON);
    gui_rect_border(x, y, w, h, GUI_COLOR_BORDER);

    uint32_t text_w = gui_text_width(text);
    uint32_t tx = x + 4;

    if (text_w < w) {
        tx = x + (w - text_w) / 2;
    }

    uint32_t ty = y + 4;

    if (h > 8) {
        ty = y + (h - 8) / 2;
    }

    gui_draw_text(tx, ty, text, GUI_COLOR_TEXT_INV);
}

static void gui_icon_folder(uint32_t x, uint32_t y, uint8_t selected) {
    (void)selected;
    uint8_t fill_main = 14;
    uint8_t fill_dark = 6;
    uint8_t border = 0;

    gui_rect(x + 2, y + 1, 8, 3, border);
    gui_rect(x + 10, y + 2, 7, 2, border);

    gui_rect(x + 3, y + 2, 6, 1, fill_main);
    gui_rect(x + 10, y + 3, 6, 1, fill_main);

    gui_rect(x + 1, y + 4, 18, 11, border);
    gui_rect(x + 2, y + 5, 16, 8, fill_main);
    gui_rect(x + 2, y + 12, 16, 2, fill_dark);
}

static void gui_icon_file(uint32_t x, uint32_t y, uint8_t selected) {
    uint8_t paper = selected ? 15 : 15;
    uint8_t border = selected ? 0 : 8;
    uint8_t fold = selected ? 7 : 7;
    uint8_t line = selected ? 0 : 8;

    /* hoja */
    gui_rect(x + 4, y + 1, 12, 15, paper);

    /* esquina doblada */
    gui_rect(x + 12, y + 1, 4, 4, fold);
    gui_putpixel(x + 12, y + 4, border);
    gui_putpixel(x + 13, y + 5, border);
    gui_putpixel(x + 14, y + 6, border);

    /* borde */
    gui_rect_border(x + 4, y + 1, 12, 15, border);

    /* líneas */
    gui_rect(x + 7, y + 7, 7, 1, line);
    gui_rect(x + 7, y + 10, 7, 1, line);
    gui_rect(x + 7, y + 13, 5, 1, line);
}

static void gui_icon_draw(fs_node_t* node, uint32_t x, uint32_t y, uint8_t selected) {
    if (!node) {
        return;
    }

    if (node->type == FS_DIR) {
        gui_icon_folder(x, y, selected);
    } else {
        gui_icon_file(x, y, selected);
    }
}

static uint32_t gui_read_raw_pixel(uint32_t x, uint32_t y) {
    if (!gui_fb_addr || x >= gui_get_width() || y >= gui_get_height()) {
        return 0;
    }

    if (gui_bpp == 32) {
        uint32_t* pixel = (uint32_t*)(gui_fb_addr + y * gui_pitch + x * 4);
        return *pixel;
    }

    if (gui_bpp == 24) {
        uint8_t* pixel = gui_fb_addr + y * gui_pitch + x * 3;

        uint32_t b = pixel[0];
        uint32_t g = pixel[1];
        uint32_t r = pixel[2];

        return 0xFF000000 | (r << 16) | (g << 8) | b;
    }

    /*
       Fallback VGA 8-bit.
       Guardamos solo el indice de color.
    */
    return gui_fb_addr[y * gui_pitch + x];
}

static void gui_write_raw_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!gui_fb_addr || x >= gui_get_width() || y >= gui_get_height()) {
        return;
    }

    if (gui_bpp == 32) {
        uint32_t* pixel = (uint32_t*)(gui_fb_addr + y * gui_pitch + x * 4);
        *pixel = color;
        return;
    }

    if (gui_bpp == 24) {
        uint8_t* pixel = gui_fb_addr + y * gui_pitch + x * 3;

        pixel[0] = color & 0xFF;
        pixel[1] = (color >> 8) & 0xFF;
        pixel[2] = (color >> 16) & 0xFF;
        return;
    }

    /*
       Fallback VGA 8-bit.
    */
    gui_fb_addr[y * gui_pitch + x] = (uint8_t)color;
}

void gui_statusbar(const char* text) {
    gui_rect(0, VGA_HEIGHT - 16, VGA_WIDTH, 16, GUI_COLOR_STATUS);
    gui_draw_text(6, VGA_HEIGHT - 12, text, GUI_COLOR_TEXT_INV);
}

static int gui_selected_item = 0;

static void gui_build_node_path(fs_node_t* node, char* out, uint32_t max) {
    if (!out || max == 0) {
        return;
    }

    for (uint32_t i = 0; i < max; i++) {
        out[i] = 0;
    }

    if (!node || node == fs_get_root()) {
        if (max > 1) {
            out[0] = '/';
            out[1] = 0;
        }
        return;
    }

    fs_node_t* stack[32];
    uint32_t count = 0;

    fs_node_t* current = node;

    while (current && current != fs_get_root() && count < 32) {
        stack[count++] = current;
        current = current->parent;
    }

    uint32_t len = 0;

    if (len < max - 1) {
        out[len++] = '/';
    }

    for (int32_t i = count - 1; i >= 0; i--) {
        uint32_t j = 0;

        while (stack[i]->name[j] && len < max - 1) {
            out[len++] = stack[i]->name[j++];
        }

        if (i != 0 && len < max - 1) {
            out[len++] = '/';
        }
    }

    out[len] = 0;
}

typedef enum {
    GUI_SCREEN_DESKTOP,
    GUI_SCREEN_FILES,
    GUI_SCREEN_FILE_PREVIEW,
    GUI_SCREEN_NEW_FILE,
    GUI_SCREEN_NEW_FOLDER,
    GUI_SCREEN_RENAME,
    GUI_SCREEN_COPY,
    GUI_SCREEN_MOVE,
    GUI_SCREEN_TEXT_EDITOR,
    GUI_SCREEN_CONTEXT_MENU,
    GUI_SCREEN_TERMINAL,
    GUI_SCREEN_INFO
} gui_screen_t;

static gui_screen_t gui_screen = GUI_SCREEN_DESKTOP;

static fs_node_t* gui_explorer_dir = 0;
static uint32_t gui_explorer_selected = 0;
static uint32_t gui_explorer_scroll = 0;
static fs_node_t* gui_preview_file = 0;
static fs_node_t* gui_rename_target = 0;
static fs_node_t* gui_transfer_target = 0;
static char gui_input_buffer[64];
static uint32_t gui_input_pos = 0;
static uint32_t gui_context_selected = 0;
static fs_node_t* gui_editor_file = 0;
static char gui_editor_buffer[512];
static uint32_t gui_editor_pos = 0;
static void gui_draw_file_preview(void);
static void gui_open_file_preview(fs_node_t* file);
static void gui_preview_handle_key(char c);
static void gui_draw_rename_dialog(void);
static void gui_open_rename_dialog(void);
static void gui_rename_handle_key(char c);
static void gui_rename_from_input(void);
static void gui_draw_transfer_dialog(const char* title);
static void gui_open_copy_dialog(void);
static void gui_open_move_dialog(void);
static void gui_copy_handle_key(char c);
static void gui_move_handle_key(char c);
static void gui_copy_from_input(void);
static void gui_move_from_input(void);
static void gui_open_text_editor(fs_node_t* file);
static void gui_draw_text_editor(void);
static void gui_text_editor_handle_key(char c);
static void gui_editor_save(void);
static void gui_draw_desktop_background(void);
static void gui_draw_taskbar(void);
static void gui_desktop_hint(const char* text);

static void gui_desktop_icon_files(uint32_t x, uint32_t y, uint8_t selected);
static void gui_desktop_icon_terminal(uint32_t x, uint32_t y, uint8_t selected);
static void gui_desktop_icon_info(uint32_t x, uint32_t y, uint8_t selected);

static void gui_draw_main_menu(void) {
    const char* items[GUI_MENU_ITEMS] = {
        "FILES",
        "TERMINAL",
        "INFO"
    };

    uint32_t x = 44;
    uint32_t y = 118;
    uint32_t w = 92;
    uint32_t h = 20;

    for (uint32_t i = 0; i < GUI_MENU_ITEMS; i++) {
        uint32_t item_y = y + i * 24;

        if ((int)i == gui_selected_item) {
            gui_rect(x, item_y, w, h, 14);
            gui_rect_border(x, item_y, w, h, 15);
            gui_draw_text(x + 8, item_y + 6, items[i], 0);
        } else {
            gui_rect(x, item_y, w, h, 8);
            gui_rect_border(x, item_y, w, h, 15);
            gui_draw_text(x + 8, item_y + 6, items[i], 15);
        }
    }
}

typedef enum {
    DESKTOP_APP_NONE,
    DESKTOP_APP_FILES,
    DESKTOP_APP_TERMINAL,
    DESKTOP_APP_INFO
} desktop_app_t;

typedef struct {
    int x;
    int y;
    int w;
    int h;
    const char* title;
    desktop_app_t app;
    int visible;
} desktop_window_t;

static void gui_terminal_redraw_input_line(desktop_window_t* win);
static void gui_draw_drag_outline(int32_t x, int32_t y, int32_t w, int32_t h);
static void gui_clamp_window_position(int32_t* x, int32_t* y, int32_t w, int32_t h);
static void gui_draw_file_preview_window(desktop_window_t* win);
static void gui_draw_text_editor_window(desktop_window_t* win);
static void gui_draw_terminal_window(desktop_window_t* win);
uint32_t gui_get_width(void);
uint32_t gui_get_height(void);
static void gui_draw_desktop_window(desktop_window_t* win);
static void gui_draw_file_explorer_window(desktop_window_t* win);
static void gui_files_window_handle_key(char c);
static uint32_t gui_files_window_row_h(desktop_window_t* win);
static uint32_t gui_files_window_max_visible(desktop_window_t* win);
static void gui_files_window_adjust_scroll(desktop_window_t* win);
static void gui_draw_file_explorer_window_row(desktop_window_t* win, uint32_t item_index);
static void gui_close_active_window(void);
static int gui_files_item_at_mouse(int32_t x, int32_t y);
static void gui_files_mouse_right_click(int32_t x, int32_t y);
static void gui_files_open_selected(void);
static void gui_mouse_left_pressed(int32_t x, int32_t y);
static void gui_mouse_left_released(int32_t x, int32_t y);
static void gui_mouse_right_pressed(int32_t x, int32_t y);
static uint32_t gui_mouse_click_counter = 0;
static uint32_t gui_last_click_counter = 0;
static uint64_t gui_read_tsc(void);

static desktop_window_t active_window = {
    120, 100, 900, 600, "Window", DESKTOP_APP_NONE, 0
};

uint32_t gui_get_width(void) {
    return gui_width;
}

uint32_t gui_get_height(void) {
    return gui_height;
}

static uint64_t gui_read_tsc(void) {
    uint32_t low;
    uint32_t high;

    __asm__ volatile ("rdtsc" : "=a"(low), "=d"(high));

    return ((uint64_t)high << 32) | low;
}

static void gui_draw_desktop_background_region(uint32_t rx, uint32_t ry, uint32_t rw, uint32_t rh) {
    uint32_t screen_w = gui_get_width();
    uint32_t screen_h = gui_get_height();

    if (screen_w == 0 || screen_h == 0) {
        return;
    }

    for (uint32_t y = ry; y < ry + rh && y < screen_h; y++) {
        uint32_t src_y = (y * BIONIC_WALLPAPER_HEIGHT) / screen_h;

        if (src_y >= BIONIC_WALLPAPER_HEIGHT) {
            src_y = BIONIC_WALLPAPER_HEIGHT - 1;
        }

        for (uint32_t x = rx; x < rx + rw && x < screen_w; x++) {
            uint32_t src_x = (x * BIONIC_WALLPAPER_WIDTH) / screen_w;

            if (src_x >= BIONIC_WALLPAPER_WIDTH) {
                src_x = BIONIC_WALLPAPER_WIDTH - 1;
            }

            gui_putpixel_rgb(x, y, bionic_wallpaper[src_y * BIONIC_WALLPAPER_WIDTH + src_x]);
        }
    }
}

static void gui_draw_desktop_icon_by_index(uint32_t index, uint8_t selected) {
    if (index == 0) {
        gui_desktop_icon_files(60, 90, selected);
    } else if (index == 1) {
        gui_desktop_icon_terminal(60, 190, selected);
    } else if (index == 2) {
        gui_desktop_icon_info(60, 290, selected);
    }
}

static void gui_clear_desktop_icon_area(uint32_t index) {
    uint32_t x = 0;
    uint32_t y = 0;

    if (index == 0) {
        x = 50;
        y = 80;
    } else if (index == 1) {
        x = 50;
        y = 180;
    } else if (index == 2) {
        x = 50;
        y = 280;
    } else {
        return;
    }

    gui_draw_desktop_background_region(x, y, 140, 80);
}

static void gui_update_desktop_selection(uint32_t old_index, uint32_t new_index) {
    gui_clear_desktop_icon_area(old_index);
    gui_clear_desktop_icon_area(new_index);

    gui_draw_desktop_icon_by_index(old_index, 0);
    gui_draw_desktop_icon_by_index(new_index, 1);
}

static void gui_redraw_desktop(void) {
    gui_draw_desktop_background();

    gui_rect(0, 0, gui_get_width(), 42, 8);
    gui_draw_text(20, 16, "Bionic Desktop", 15);

    gui_desktop_icon_files(60, 90, desktop_selected_icon == 0);
    gui_desktop_icon_terminal(60, 190, desktop_selected_icon == 1);
    gui_desktop_icon_info(60, 290, desktop_selected_icon == 2);

    gui_draw_desktop_window(&active_window);

    gui_draw_taskbar();
}

static void gui_desktop_hint(const char* text) {
    gui_rect(0, VGA_HEIGHT - 38, VGA_WIDTH, 14, 8);
    gui_draw_text(6, VGA_HEIGHT - 35, text, 15);
}

/* void gui_demo(void) {
    input_set_mode(INPUT_MODE_GUI);

    gui_set_mode13h();

    gui_redraw_desktop();

    gui_clear(GUI_COLOR_BG);

    gui_rect(0, 0, VGA_WIDTH, 24, GUI_COLOR_TOPBAR);
    gui_draw_text(8, 8, "Bionic GUI Preview", GUI_COLOR_TEXT_INV);

    gui_window(24, 36, 272, 126, "Bionic Desktop");

    gui_label(40, 66, "Use W/S to move", GUI_COLOR_TEXT);
    gui_label(40, 80, "Enter to select", GUI_COLOR_TEXT);

    gui_draw_main_menu();

    gui_statusbar("GUI ready - W/S move, Enter select");

    gui_rect(286, 170, 8, 8, 15);

    gui_redraw_desktop();
}
    */

void gui_demo(void) {
    input_set_mode(INPUT_MODE_GUI);

    if (!gui_using_framebuffer) {
        gui_set_mode13h();
        gui_fb_addr = (uint8_t*)0xA0000;
        gui_width = 320;
        gui_height = 200;
        gui_pitch = 320;
        gui_bpp = 8;
    }

    gui_screen = GUI_SCREEN_DESKTOP;
    desktop_selected_icon = 0;

    gui_explorer_dir = fs_get_root();
    gui_explorer_selected = 0;

    gui_redraw_desktop();
}

static void gui_draw_file_explorer(void) {
    if (active_window.visible && active_window.app == DESKTOP_APP_FILES) {
        gui_redraw_desktop();
        return;
    }

    gui_clear(GUI_COLOR_BG);

    gui_rect(0, 0, VGA_WIDTH, 24, GUI_COLOR_TOPBAR);
    gui_draw_text(8, 8, "Bionic File Explorer", GUI_COLOR_TEXT_INV);

    gui_window(16, 32, 288, 142, "Files");

    char path[128];
    gui_build_node_path(gui_explorer_dir, path, 128);

    gui_label(26, 58, "Path:", GUI_COLOR_TEXT);
    gui_label(74, 58, path, GUI_COLOR_TEXT);
    uint32_t count = fs_count_children(gui_explorer_dir);

    if (count == 0) {
        gui_label(38, 86, "This directory is empty", GUI_COLOR_TEXT);
    } else {
uint32_t max_visible = 5;

for (uint32_t i = 0; i < count && i < max_visible; i++) {
    fs_node_t* child = fs_get_child_at(gui_explorer_dir, i);

    if (!child) {
        continue;
    }

    uint32_t y = 78 + i * 18;

    uint8_t selected = (i == gui_explorer_selected);
    uint8_t text_color = selected ? 0 : GUI_COLOR_TEXT;

    if (selected) {
        gui_rect(28, y - 5, 260, 17, 14);
        gui_draw_text(32, y, ">", 0);
    } else {
        gui_rect(28, y - 5, 260, 17, GUI_COLOR_WINDOW);
    }

    gui_icon_draw(child, 44, y - 4, selected);

    gui_draw_text(72, y, child->name, text_color);

    if (child->type == FS_FILE) {
        gui_draw_text(210, y, "FILE", text_color);
    } else {
        gui_draw_text(210, y, "DIR", text_color);
    }
}

        if (count > max_visible) {
            gui_label(38, 164, "More items not shown", GUI_COLOR_TEXT);
        }
    }

    gui_statusbar("W/S move Enter open Space menu Backspace up Q desktop");
}

static void gui_desktop_icon_box(uint32_t x, uint32_t y, const char* label, uint8_t selected) {
    /*
       Selección solo alrededor del icono, no del texto.
    */
    if (selected) {
        gui_rect(x - 4, y - 4, 40, 36, 14);
        gui_rect_border(x - 4, y - 4, 40, 36, 15);
    }

    /*
       Caja del icono.
    */
    gui_panel(x, y, 32, 28, selected ? 15 : 7, 0);

    /*
       Limpiamos zona del texto para que no quede mezclado con la selección.
    */
    gui_rect(x - 4, y + 32, 72, 12, 9);

    /*
       Texto siempre visible.
       Si está seleccionado, lo dejamos blanco igualmente.
    */
    gui_draw_text(x + 2, y + 34, label, 15);
}

static void gui_desktop_icon_files(uint32_t x, uint32_t y, uint8_t selected) {
    gui_desktop_icon_box(x, y, "Files", selected);
    gui_icon_folder(x + 6, y + 6, selected);
}

static void gui_desktop_icon_terminal(uint32_t x, uint32_t y, uint8_t selected) {
    gui_desktop_icon_box(x, y, "Term", selected);

    gui_rect(x + 6, y + 7, 20, 14, 0);
    gui_rect_border(x + 6, y + 7, 20, 14, 15);
    gui_draw_text(x + 9, y + 10, ">", 10);
}

static void gui_desktop_icon_info(uint32_t x, uint32_t y, uint8_t selected) {
    gui_desktop_icon_box(x, y, "Info", selected);

    gui_rect(x + 9, y + 4, 14, 20, 9);
    gui_rect_border(x + 9, y + 4, 14, 20, 15);
    gui_draw_text(x + 13, y + 10, "I", 15);
}

static void gui_open_file_explorer(void) {
    gui_screen = GUI_SCREEN_FILES;
    gui_explorer_dir = fs_get_root();
    gui_explorer_selected = 0;

    gui_draw_file_explorer();
}

static void gui_files_open_selected(void) {
    fs_node_t* selected = fs_get_child_at(gui_explorer_dir, gui_explorer_selected);

    if (!selected) {
        return;
    }

    if (selected->type == FS_DIR) {
        gui_explorer_dir = selected;
        gui_explorer_selected = 0;
        gui_explorer_scroll = 0;

        gui_draw_desktop_window(&active_window);
    } else {
        gui_preview_file = selected;
        gui_screen = GUI_SCREEN_FILE_PREVIEW;

        gui_draw_desktop_window(&active_window);
    }
}

static void gui_files_handle_key(char c) {
    uint32_t count = fs_count_children(gui_explorer_dir);

    if (c == 'w' || c == 'W') {
        if (count > 0) {
            if (gui_explorer_selected > 0) {
                gui_explorer_selected--;
            } else {
                gui_explorer_selected = count - 1;
            }
        }

        gui_draw_file_explorer();
    }

    else if (c == 's' || c == 'S') {
        if (count > 0) {
            gui_explorer_selected++;

            if (gui_explorer_selected >= count) {
                gui_explorer_selected = 0;
            }
        }

        gui_draw_file_explorer();
    }

else if (c == '\n') {
    gui_files_open_selected();
}

/* else if (c == '\n') {
    fs_node_t* selected = fs_get_child_at(gui_explorer_dir, gui_explorer_selected);

        if (!selected) {
            gui_statusbar("No item selected");
            return;
        }

        if (selected->type == FS_DIR) {
            gui_explorer_dir = selected;
            gui_explorer_selected = 0;
            gui_draw_file_explorer();
        } else {
            gui_open_file_preview(selected);
        }
    }
        */

    else if (c == '\b') {
        if (gui_explorer_dir && gui_explorer_dir->parent && gui_explorer_dir != fs_get_root()) {
            gui_explorer_dir = gui_explorer_dir->parent;
            gui_explorer_selected = 0;
        }

        gui_draw_file_explorer();
    }

else if (c == ' ') {
    gui_open_context_menu();
}

    else if (c == 'q' || c == 'Q') {
        gui_screen = GUI_SCREEN_DESKTOP;
        gui_redraw_desktop();
        gui_statusbar("Returned to desktop menu");
    }
}

static int gui_files_item_at_mouse(int32_t x, int32_t y) {
    if (!active_window.visible || active_window.app != DESKTOP_APP_FILES) {
        return -1;
    }

    /*
       No queremos seleccionar cuando estamos en preview/editor/context menu.
       Solo en vista normal de archivos.
    */
    if (gui_screen != GUI_SCREEN_FILES && gui_screen != GUI_SCREEN_DESKTOP) {
        return -1;
    }

    int32_t list_x = active_window.x + 28;
    int32_t list_y = active_window.y + 92;
    int32_t list_w = active_window.w - 56;
    int32_t row_h = 26;

    if (x < list_x || x >= list_x + list_w) {
        return -1;
    }

    if (y < list_y) {
        return -1;
    }

    int32_t local_y = y - list_y;
    int32_t row = local_y / row_h;

    if (row < 0) {
        return -1;
    }

    uint32_t max_visible = 0;

    if (active_window.h > 160) {
        max_visible = (active_window.h - 150) / row_h;
    }

    if (max_visible == 0) {
        max_visible = 1;
    }

    if ((uint32_t)row >= max_visible) {
        return -1;
    }

    uint32_t index = gui_explorer_scroll + (uint32_t)row;
    uint32_t count = fs_count_children(gui_explorer_dir);

    if (index >= count) {
        return -1;
    }

    return (int)index;
}

static void gui_files_mouse_left_click(int32_t x, int32_t y) {
    int index = gui_files_item_at_mouse(x, y);

    if (index < 0) {
        return;
    }

    uint32_t old_selected = gui_explorer_selected;
    gui_explorer_selected = (uint32_t)index;

    if (old_selected != gui_explorer_selected) {
        gui_draw_desktop_window(&active_window);
    }

    uint64_t now = gui_read_tsc();

    int32_t dx = x - gui_last_click_x;
    int32_t dy = y - gui_last_click_y;

    int same_item = gui_last_click_item == index;
    int close_position = dx > -8 && dx < 8 && dy > -8 && dy < 8;
    int fast_enough = gui_last_click_time != 0 &&
                      (now - gui_last_click_time) < GUI_DOUBLE_CLICK_TSC_LIMIT;

    if (same_item && close_position && fast_enough) {
        gui_last_click_time = 0;
        gui_last_click_item = -1;

        gui_files_open_selected();
        return;
    }

    gui_last_click_time = now;
    gui_last_click_x = x;
    gui_last_click_y = y;
    gui_last_click_item = index;
}

static void gui_files_mouse_right_click(int32_t x, int32_t y) {
    int index = gui_files_item_at_mouse(x, y);

    if (index >= 0) {
        gui_explorer_selected = (uint32_t)index;
        gui_draw_desktop_window(&active_window);
    }

    /*
       Igual que pulsar Space en el explorer.
    */
    gui_open_context_menu();
}

static void gui_draw_file_preview_window(desktop_window_t* win) {
    if (!win || !win->visible || win->app != DESKTOP_APP_FILES) {
        return;
    }

    uint32_t content_x = win->x + 24;
    uint32_t content_y = win->y + 46;
    uint32_t content_w = win->w > 48 ? win->w - 48 : win->w;
    uint32_t panel_y = content_y + 62;
    uint32_t footer_y = win->y + win->h - 28;
    uint32_t content_h = footer_y > panel_y + 14 ? footer_y - panel_y - 14 : 60;

    gui_draw_text(content_x, content_y, "File preview", 0);

    if (!gui_preview_file || gui_preview_file->type != FS_FILE) {
        gui_draw_text(content_x + 16, content_y + 42, "No file selected", 0);
        gui_draw_text(content_x + 16, win->y + win->h - 28, "Backspace return", 8);
        return;
    }

    char path[128];
    gui_build_node_path(gui_preview_file, path, 128);

    gui_draw_text(content_x, content_y + 24, "File:", 0);
    gui_draw_text(content_x + 52, content_y + 24, gui_preview_file->name, 0);

    gui_draw_text(content_x, content_y + 42, "Path:", 0);
    gui_draw_text(content_x + 52, content_y + 42, path, 0);

    gui_panel(content_x, panel_y, content_w, content_h, 15, 0);

    if (gui_preview_file->size == 0) {
        gui_draw_text(content_x + 16, panel_y + 16, "[empty file]", 0);
    } else {
        gui_draw_wrapped_text(
            content_x + 16,
            panel_y + 16,
            (content_w > 32 ? (content_w - 32) / 8 : 1),
            (content_h > 20 ? (content_h - 20) / 10 : 1),
            gui_preview_file->content,
            0
        );
    }

    gui_draw_text(content_x + 16, win->y + win->h - 28, "E edit  Backspace return  Q close", 8);
}

static void gui_draw_file_preview(void) {
    if (active_window.visible && active_window.app == DESKTOP_APP_FILES) {
        gui_draw_desktop_window(&active_window);
        return;
    }

    gui_clear(GUI_COLOR_BG);

    gui_rect(0, 0, VGA_WIDTH, 24, GUI_COLOR_TOPBAR);
    gui_draw_text(8, 8, "Bionic File Preview", GUI_COLOR_TEXT_INV);

    gui_window(16, 32, 288, 142, "Preview");

    if (!gui_preview_file || gui_preview_file->type != FS_FILE) {
        gui_label(34, 64, "No file selected", GUI_COLOR_TEXT);
        gui_statusbar("E edit  Backspace return  Q menu");
        return;
    }

    char path[128];
    gui_build_node_path(gui_preview_file, path, 128);

    gui_label(26, 58, "File:", GUI_COLOR_TEXT);
    gui_label(74, 58, gui_preview_file->name, GUI_COLOR_TEXT);

    gui_label(26, 70, "Path:", GUI_COLOR_TEXT);
    gui_label(74, 70, path, GUI_COLOR_TEXT);

    gui_panel(26, 86, 268, 72, 15, 0);

    if (gui_preview_file->size == 0) {
        gui_label(34, 96, "[empty file]", 0);
    } else {
        gui_draw_wrapped_text(34, 96, 30, 6, gui_preview_file->content, 0);
    }

    gui_statusbar("E edit  Backspace return  Q menu");
}

static void gui_preview_handle_key(char c) {
    if (c == '\b') {
        if (active_window.visible && active_window.app == DESKTOP_APP_FILES) {
            gui_screen = GUI_SCREEN_DESKTOP;
            gui_draw_desktop_window(&active_window);
        } else {
            gui_screen = GUI_SCREEN_FILES;
            gui_draw_file_explorer();
        }
        return;
    }

    if (c == 'q' || c == 'Q') {
        if (active_window.visible && active_window.app == DESKTOP_APP_FILES) {
            gui_screen = GUI_SCREEN_DESKTOP;
            gui_close_active_window();
        } else {
            gui_screen = GUI_SCREEN_DESKTOP;
            gui_redraw_desktop();
            gui_statusbar("Returned to desktop menu");
        }
        return;
    }

    if (c == 'e' || c == 'E') {
        if (gui_preview_file && gui_preview_file->type == FS_FILE) {
            gui_open_text_editor(gui_preview_file);
        }
        return;
    }
}

static void gui_open_file_preview(fs_node_t* file) {
    if (!file || file->type != FS_FILE) {
        return;
    }

    gui_preview_file = file;
    gui_screen = GUI_SCREEN_FILE_PREVIEW;

    gui_draw_file_preview();
}

static void gui_input_clear(void) {
    for (uint32_t i = 0; i < 64; i++) {
        gui_input_buffer[i] = 0;
    }

    gui_input_pos = 0;
}

static void gui_input_add_char(char c) {
    if (gui_input_pos >= 63) {
        return;
    }

    gui_input_buffer[gui_input_pos++] = c;
    gui_input_buffer[gui_input_pos] = 0;
}

static void gui_input_backspace(void) {
    if (gui_input_pos == 0) {
        return;
    }

    gui_input_pos--;
    gui_input_buffer[gui_input_pos] = 0;
}


static void gui_get_dialog_input_rect(uint32_t* out_x, uint32_t* out_y, uint32_t* out_w, uint32_t* out_h,
                                      uint32_t* out_text_x, uint32_t* out_text_y) {
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t w = 0;

    if (gui_screen == GUI_SCREEN_NEW_FILE || gui_screen == GUI_SCREEN_NEW_FOLDER) {
        x = active_window.visible ? active_window.x + (active_window.w / 2) - 180 : 42;
        y = active_window.visible ? active_window.y + 150 : 70;
        w = active_window.visible ? 360 : 236;

        *out_x = x + 76;
        *out_y = y + 38;
        *out_w = w - 102;
        *out_h = 20;
        *out_text_x = x + 84;
        *out_text_y = y + 45;
        return;
    }

    if (gui_screen == GUI_SCREEN_RENAME) {
        x = active_window.visible ? active_window.x + (active_window.w / 2) - 200 : 42;
        y = active_window.visible ? active_window.y + 145 : 66;
        w = active_window.visible ? 400 : 236;

        *out_x = x + 76;
        *out_y = y + 60;
        *out_w = w - 102;
        *out_h = 20;
        *out_text_x = x + 84;
        *out_text_y = y + 67;
        return;
    }

    if (gui_screen == GUI_SCREEN_COPY || gui_screen == GUI_SCREEN_MOVE) {
        x = active_window.visible ? active_window.x + (active_window.w / 2) - 220 : 34;
        y = active_window.visible ? active_window.y + 145 : 58;
        w = active_window.visible ? 440 : 252;

        *out_x = x + 82;
        *out_y = y + 64;
        *out_w = w - 110;
        *out_h = 20;
        *out_text_x = x + 90;
        *out_text_y = y + 71;
        return;
    }

    *out_x = 0;
    *out_y = 0;
    *out_w = 0;
    *out_h = 0;
    *out_text_x = 0;
    *out_text_y = 0;
}

static void gui_redraw_dialog_input_field(void) {
    uint32_t x, y, w, h, text_x, text_y;

    gui_get_dialog_input_rect(&x, &y, &w, &h, &text_x, &text_y);

    if (w == 0 || h == 0) {
        return;
    }

    gui_rect(x, y, w, h, 15);
    gui_rect_border(x, y, w, h, 0);
    gui_draw_text(text_x, text_y, gui_input_buffer, 0);
}

static void gui_draw_new_folder_dialog(void) {
    gui_draw_file_explorer();

    uint32_t x = active_window.visible ? active_window.x + (active_window.w / 2) - 180 : 42;
    uint32_t y = active_window.visible ? active_window.y + 150 : 70;
    uint32_t w = active_window.visible ? 360 : 236;
    uint32_t h = active_window.visible ? 110 : 62;

    gui_panel(x, y, w, h, 7, 15);
    gui_rect(x, y, w, 24, 9);
    gui_draw_text(x + 12, y + 9, "Create new folder", 15);

    gui_draw_text(x + 18, y + 44, "Name:", 0);

    gui_rect(x + 76, y + 38, w - 102, 20, 15);
    gui_rect_border(x + 76, y + 38, w - 102, 20, 0);
    gui_draw_text(x + 84, y + 45, gui_input_buffer, 0);

    gui_draw_text(x + 18, y + 78, "Enter OK   Backspace edit/cancel", 0);
}


static void gui_open_new_folder_dialog(void) {
    gui_screen = GUI_SCREEN_NEW_FOLDER;
    gui_input_clear();
    gui_draw_new_folder_dialog();
}

static void gui_draw_rename_dialog(void) {
    gui_draw_file_explorer();

    uint32_t x = active_window.visible ? active_window.x + (active_window.w / 2) - 200 : 42;
    uint32_t y = active_window.visible ? active_window.y + 145 : 66;
    uint32_t w = active_window.visible ? 400 : 236;
    uint32_t h = active_window.visible ? 130 : 72;

    gui_panel(x, y, w, h, 7, 15);
    gui_rect(x, y, w, 24, 9);
    gui_draw_text(x + 12, y + 9, "Rename item", 15);

    if (!gui_rename_target) {
        gui_draw_text(x + 18, y + 50, "No item selected", 0);
        gui_draw_text(x + 18, y + 92, "Backspace cancel", 0);
        return;
    }

    gui_draw_text(x + 18, y + 42, "Old:", 0);
    gui_draw_text(x + 76, y + 42, gui_rename_target->name, 0);

    gui_draw_text(x + 18, y + 66, "New:", 0);

    gui_rect(x + 76, y + 60, w - 102, 20, 15);
    gui_rect_border(x + 76, y + 60, w - 102, 20, 0);
    gui_draw_text(x + 84, y + 67, gui_input_buffer, 0);

    gui_draw_text(x + 18, y + 100, "Enter OK   Backspace edit/cancel", 0);
}


static void gui_open_rename_dialog(void) {
    uint32_t count = fs_count_children(gui_explorer_dir);

    if (count == 0) {
        gui_screen = GUI_SCREEN_FILES;
        gui_draw_file_explorer();
        gui_statusbar("Nothing to rename");
        return;
    }

    gui_rename_target = fs_get_child_at(gui_explorer_dir, gui_explorer_selected);

    if (!gui_rename_target) {
        gui_screen = GUI_SCREEN_FILES;
        gui_draw_file_explorer();
        gui_statusbar("No item selected");
        return;
    }

    gui_screen = GUI_SCREEN_RENAME;
    gui_input_clear();

    /*
       Opcional: rellenar el input con el nombre actual.
       Así puedes editar visualmente borrando con Backspace.
    */
    uint32_t i = 0;
    while (gui_rename_target->name[i] && i < 63) {
        gui_input_buffer[i] = gui_rename_target->name[i];
        i++;
    }

    gui_input_buffer[i] = 0;
    gui_input_pos = i;

    gui_draw_rename_dialog();
}

static void gui_build_destination_path(const char* input, char* out, uint32_t max) {
    if (!out || max == 0) {
        return;
    }

    for (uint32_t i = 0; i < max; i++) {
        out[i] = 0;
    }

    if (!input || input[0] == '\0') {
        return;
    }

    /*
       Si empieza por /, ya es ruta absoluta.
    */
    if (input[0] == '/') {
        uint32_t i = 0;

        while (input[i] && i < max - 1) {
            out[i] = input[i];
            i++;
        }

        out[i] = 0;
        return;
    }

    /*
       Si es relativa, la hacemos relativa a la carpeta actual del explorador,
       no a la carpeta actual de la shell.
    */
    char current_path[128];
    gui_build_node_path(gui_explorer_dir, current_path, 128);

    uint32_t pos = 0;

    uint32_t i = 0;
    while (current_path[i] && pos < max - 1) {
        out[pos++] = current_path[i++];
    }

    if (!(pos == 1 && out[0] == '/')) {
        if (pos < max - 1) {
            out[pos++] = '/';
        }
    }

    i = 0;
    while (input[i] && pos < max - 1) {
        out[pos++] = input[i++];
    }

    out[pos] = 0;
}

static void gui_draw_transfer_dialog(const char* title) {
    gui_draw_file_explorer();

    uint32_t x = active_window.visible ? active_window.x + (active_window.w / 2) - 220 : 34;
    uint32_t y = active_window.visible ? active_window.y + 145 : 58;
    uint32_t w = active_window.visible ? 440 : 252;
    uint32_t h = active_window.visible ? 135 : 88;

    gui_panel(x, y, w, h, 7, 15);
    gui_rect(x, y, w, 24, 9);
    gui_draw_text(x + 12, y + 9, title, 15);

    if (!gui_transfer_target) {
        gui_draw_text(x + 18, y + 54, "No item selected", 0);
        gui_draw_text(x + 18, y + 100, "Backspace cancel", 0);
        return;
    }

    gui_draw_text(x + 18, y + 44, "Item:", 0);
    gui_draw_text(x + 82, y + 44, gui_transfer_target->name, 0);

    gui_draw_text(x + 18, y + 70, "Dest:", 0);

    gui_rect(x + 82, y + 64, w - 110, 20, 15);
    gui_rect_border(x + 82, y + 64, w - 110, 20, 0);
    gui_draw_text(x + 90, y + 71, gui_input_buffer, 0);

    gui_draw_text(x + 18, y + 106, "Enter OK   Backspace edit/cancel", 0);
}


static void gui_open_copy_dialog(void) {
    uint32_t count = fs_count_children(gui_explorer_dir);

    if (count == 0) {
        gui_screen = GUI_SCREEN_FILES;
        gui_draw_file_explorer();
        gui_statusbar("Nothing to copy");
        return;
    }

    gui_transfer_target = fs_get_child_at(gui_explorer_dir, gui_explorer_selected);

    if (!gui_transfer_target) {
        gui_screen = GUI_SCREEN_FILES;
        gui_draw_file_explorer();
        gui_statusbar("No item selected");
        return;
    }

    gui_screen = GUI_SCREEN_COPY;
    gui_input_clear();
    gui_draw_transfer_dialog("Copy item");
}

static void gui_open_move_dialog(void) {
    uint32_t count = fs_count_children(gui_explorer_dir);

    if (count == 0) {
        gui_screen = GUI_SCREEN_FILES;
        gui_draw_file_explorer();
        gui_statusbar("Nothing to move");
        return;
    }

    gui_transfer_target = fs_get_child_at(gui_explorer_dir, gui_explorer_selected);

    if (!gui_transfer_target) {
        gui_screen = GUI_SCREEN_FILES;
        gui_draw_file_explorer();
        gui_statusbar("No item selected");
        return;
    }

    gui_screen = GUI_SCREEN_MOVE;
    gui_input_clear();
    gui_draw_transfer_dialog("Move item");
}

static void gui_copy_from_input(void) {
    if (!gui_transfer_target) {
        gui_screen = GUI_SCREEN_FILES;
        gui_draw_file_explorer();
        gui_statusbar("No item selected");
        return;
    }

    if (gui_input_buffer[0] == '\0') {
        gui_draw_transfer_dialog("Copy item");
        gui_statusbar("Destination cannot be empty");
        return;
    }

    if (gui_transfer_target->type == FS_DIR) {
        gui_draw_transfer_dialog("Copy item");
        gui_statusbar("Copy folders not supported yet");
        return;
    }

    char src_path[128];
    char dst_path[192];

    gui_build_node_path(gui_transfer_target, src_path, 128);
    gui_build_destination_path(gui_input_buffer, dst_path, 192);

    fs_cp(src_path, dst_path);

    gui_transfer_target = 0;
    gui_screen = GUI_SCREEN_FILES;
    gui_draw_file_explorer();
    gui_statusbar("Item copied");
}

static void gui_move_from_input(void) {
    if (!gui_transfer_target) {
        gui_screen = GUI_SCREEN_FILES;
        gui_draw_file_explorer();
        gui_statusbar("No item selected");
        return;
    }

    if (gui_input_buffer[0] == '\0') {
        gui_draw_transfer_dialog("Move item");
        gui_statusbar("Destination cannot be empty");
        return;
    }

    char src_path[128];
    char dst_path[192];

    gui_build_node_path(gui_transfer_target, src_path, 128);
    gui_build_destination_path(gui_input_buffer, dst_path, 192);

    fs_mv(src_path, dst_path);

    gui_transfer_target = 0;
    gui_explorer_selected = 0;
    gui_screen = GUI_SCREEN_FILES;
    gui_draw_file_explorer();
    gui_statusbar("Item moved");
}

static int gui_is_path_char(char c) {
    return (
        (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '.' ||
        c == '_' ||
        c == '-' ||
        c == '/'
    );
}

static void gui_copy_handle_key(char c) {
    if (c == '\n') {
        gui_copy_from_input();
        return;
    }

    if (c == '\b') {
        if (gui_input_pos == 0) {
            gui_transfer_target = 0;
            gui_screen = GUI_SCREEN_DESKTOP;
            gui_draw_desktop_window(&active_window);
            return;
        }

        gui_input_backspace();
        gui_redraw_dialog_input_field();
        return;
    }

    if (gui_is_path_char(c)) {
        gui_input_add_char(c);
        gui_redraw_dialog_input_field();
    }
}

static void gui_move_handle_key(char c) {
    if (c == '\n') {
        gui_move_from_input();
        return;
    }

    if (c == '\b') {
        if (gui_input_pos == 0) {
            gui_transfer_target = 0;
            gui_screen = GUI_SCREEN_DESKTOP;
            gui_draw_desktop_window(&active_window);
            return;
        }

        gui_input_backspace();
        gui_redraw_dialog_input_field();
        return;
    }

    if (gui_is_path_char(c)) {
        gui_input_add_char(c);
        gui_redraw_dialog_input_field();
    }
}

static void gui_rename_from_input(void) {
    if (!gui_rename_target) {
        gui_screen = GUI_SCREEN_FILES;
        gui_draw_file_explorer();
        gui_statusbar("No item selected");
        return;
    }

    if (gui_input_buffer[0] == '\0') {
        gui_draw_rename_dialog();
        gui_statusbar("Name cannot be empty");
        return;
    }

    char old_path[128];
    gui_build_node_path(gui_rename_target, old_path, 128);

    fs_rename(old_path, gui_input_buffer);

    gui_rename_target = 0;
    gui_screen = GUI_SCREEN_FILES;
    gui_draw_file_explorer();
    gui_statusbar("Item renamed");
}

static void gui_rename_handle_key(char c) {
    if (c == '\n') {
        gui_rename_from_input();
        return;
    }

    if (c == '\b') {
        if (gui_input_pos == 0) {
            gui_rename_target = 0;
            gui_screen = GUI_SCREEN_DESKTOP;
            gui_draw_desktop_window(&active_window);
            return;
        }

        gui_input_backspace();
        gui_redraw_dialog_input_field();
        return;
    }

    if (
        (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '.' ||
        c == '_' ||
        c == '-'
    ) {
        gui_input_add_char(c);
        gui_redraw_dialog_input_field();
    }
}

static void gui_draw_new_file_dialog(void) {
    gui_draw_file_explorer();

    uint32_t x = active_window.visible ? active_window.x + (active_window.w / 2) - 180 : 42;
    uint32_t y = active_window.visible ? active_window.y + 150 : 70;
    uint32_t w = active_window.visible ? 360 : 236;
    uint32_t h = active_window.visible ? 110 : 62;

    gui_panel(x, y, w, h, 7, 15);
    gui_rect(x, y, w, 24, 9);
    gui_draw_text(x + 12, y + 9, "Create new file", 15);

    gui_draw_text(x + 18, y + 44, "Name:", 0);

    gui_rect(x + 76, y + 38, w - 102, 20, 15);
    gui_rect_border(x + 76, y + 38, w - 102, 20, 0);
    gui_draw_text(x + 84, y + 45, gui_input_buffer, 0);

    gui_draw_text(x + 18, y + 78, "Enter OK   Backspace edit/cancel", 0);
}


static void gui_open_new_file_dialog(void) {
    gui_screen = GUI_SCREEN_NEW_FILE;
    gui_input_clear();
    gui_draw_new_file_dialog();
}

static void gui_create_folder_from_input(void) {
    if (gui_input_buffer[0] == '\0') {
        gui_draw_new_folder_dialog();
        gui_statusbar("Folder name cannot be empty");
        return;
    }

    char dir_path[128];
    char full_path[192];

    gui_build_node_path(gui_explorer_dir, dir_path, 128);

    for (uint32_t i = 0; i < 192; i++) {
        full_path[i] = 0;
    }

    uint32_t pos = 0;

    uint32_t i = 0;
    while (dir_path[i] && pos < 191) {
        full_path[pos++] = dir_path[i++];
    }

    if (!(pos == 1 && full_path[0] == '/')) {
        if (pos < 191) {
            full_path[pos++] = '/';
        }
    }

    i = 0;
    while (gui_input_buffer[i] && pos < 191) {
        full_path[pos++] = gui_input_buffer[i++];
    }

    full_path[pos] = 0;

    fs_mkdir(full_path);

    gui_screen = GUI_SCREEN_FILES;
    gui_explorer_selected = 0;
    gui_draw_file_explorer();
    gui_statusbar("Folder created");
}

static void gui_create_file_from_input(void) {
    if (gui_input_buffer[0] == '\0') {
        gui_draw_new_file_dialog();
        gui_statusbar("File name cannot be empty");
        return;
    }

    char dir_path[128];
    char full_path[192];

    gui_build_node_path(gui_explorer_dir, dir_path, 128);

    for (uint32_t i = 0; i < 192; i++) {
        full_path[i] = 0;
    }

    uint32_t pos = 0;

    uint32_t i = 0;
    while (dir_path[i] && pos < 191) {
        full_path[pos++] = dir_path[i++];
    }

    if (!(pos == 1 && full_path[0] == '/')) {
        if (pos < 191) {
            full_path[pos++] = '/';
        }
    }

    i = 0;
    while (gui_input_buffer[i] && pos < 191) {
        full_path[pos++] = gui_input_buffer[i++];
    }

    full_path[pos] = 0;

    fs_touch(full_path);

    gui_screen = GUI_SCREEN_FILES;
    gui_explorer_selected = 0;
    gui_draw_file_explorer();
    gui_statusbar("File created");
}

static void gui_delete_selected_item(void) {
    uint32_t count = fs_count_children(gui_explorer_dir);

    if (count == 0) {
        gui_screen = GUI_SCREEN_FILES;
        gui_draw_file_explorer();
        gui_statusbar("Nothing to delete");
        return;
    }

    fs_node_t* selected = fs_get_child_at(gui_explorer_dir, gui_explorer_selected);

    if (!selected) {
        gui_screen = GUI_SCREEN_FILES;
        gui_draw_file_explorer();
        gui_statusbar("No item selected");
        return;
    }

    char path[128];
    gui_build_node_path(selected, path, 128);

    if (selected->type == FS_DIR) {
        /*
           Si es carpeta, usamos borrado recursivo.
           Esto borra carpetas vacias y tambien carpetas con contenido.
        */
        fs_rm_recursive(path);
    } else {
        fs_rm(path);
    }

    uint32_t new_count = fs_count_children(gui_explorer_dir);

    if (new_count == 0) {
        gui_explorer_selected = 0;
    } else if (gui_explorer_selected >= new_count) {
        gui_explorer_selected = new_count - 1;
    }

    gui_screen = GUI_SCREEN_FILES;
    gui_draw_file_explorer();
    gui_statusbar("Item deleted");
}

static void gui_new_folder_handle_key(char c) {
    if (c == '\n') {
        gui_create_folder_from_input();
        return;
    }

    if (c == '\b') {
        if (gui_input_pos == 0) {
            gui_screen = GUI_SCREEN_DESKTOP;
            gui_draw_desktop_window(&active_window);
            return;
        }

        gui_input_backspace();
        gui_redraw_dialog_input_field();
        return;
    }

    if (
        (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '.' ||
        c == '_' ||
        c == '-'
    ) {
        gui_input_add_char(c);
        gui_redraw_dialog_input_field();
    }
}

static void gui_new_file_handle_key(char c) {
    if (c == '\n') {
        gui_create_file_from_input();
        return;
    }

    if (c == '\b') {
        if (gui_input_pos == 0) {
            gui_screen = GUI_SCREEN_DESKTOP;
            gui_draw_desktop_window(&active_window);
            return;
        }

        gui_input_backspace();
        gui_redraw_dialog_input_field();
        return;
    }

    /*
       Permitimos caracteres simples para nombres.
       Puedes ampliar esto luego.
    */
    if (
        (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '.' ||
        c == '_' ||
        c == '-'
    ) {
        gui_input_add_char(c);
        gui_redraw_dialog_input_field();
    }
}

static void gui_draw_desktop_background(void) {
    gui_draw_desktop_background_region(0, 0, gui_get_width(), gui_get_height());
}

static void gui_draw_taskbar(void) {
    uint32_t w = gui_get_width();
    uint32_t h = gui_get_height();

    uint32_t bar_h = 48;
    uint32_t y = h - bar_h;

    gui_rect(0, y, w, bar_h, 8);
    gui_rect(0, y, w, 2, 15);

    gui_rect(16, y + 8, 110, 32, 7);
    gui_rect_border(16, y + 8, 110, 32, 0);
    gui_draw_text(36, y + 20, "Start", 0);

    gui_rect(150, y + 8, 110, 32, 9);
    gui_rect_border(150, y + 8, 110, 32, 15);
    gui_draw_text(174, y + 20, "Files", 15);

    gui_rect(276, y + 8, 130, 32, 0);
    gui_rect_border(276, y + 8, 130, 32, 15);
    gui_draw_text(300, y + 20, "Terminal", 10);

    gui_draw_text(w - 120, y + 20, "Bionic", 15);
}


static uint32_t gui_files_window_row_h(desktop_window_t* win) {
    (void)win;
    return 22;
}

static uint32_t gui_files_window_max_visible(desktop_window_t* win) {
    if (!win) {
        return 1;
    }

    uint32_t content_h = win->h > 76 ? win->h - 86 : win->h;
    uint32_t row_h = gui_files_window_row_h(win);
    uint32_t max_visible = 1;

    if (content_h > 42) {
        max_visible = (content_h - 22) / row_h;
    }

    if (max_visible < 1) {
        max_visible = 1;
    }

    return max_visible;
}

static void gui_files_window_adjust_scroll(desktop_window_t* win) {
    uint32_t max_visible = gui_files_window_max_visible(win);

    if (gui_explorer_selected < gui_explorer_scroll) {
        gui_explorer_scroll = gui_explorer_selected;
    }

    if (gui_explorer_selected >= gui_explorer_scroll + max_visible) {
        gui_explorer_scroll = gui_explorer_selected - max_visible + 1;
    }
}

static void gui_draw_file_explorer_window_row(desktop_window_t* win, uint32_t item_index) {
    if (!win || !win->visible || win->app != DESKTOP_APP_FILES) {
        return;
    }

    uint32_t count = fs_count_children(gui_explorer_dir);
    uint32_t max_visible = gui_files_window_max_visible(win);

    if (item_index >= count) {
        return;
    }

    if (item_index < gui_explorer_scroll || item_index >= gui_explorer_scroll + max_visible) {
        return;
    }

    fs_node_t* child = fs_get_child_at(gui_explorer_dir, item_index);

    if (!child) {
        return;
    }

    uint32_t content_x = win->x + 24;
    uint32_t content_y = win->y + 46;
    uint32_t content_w = win->w > 48 ? win->w - 48 : win->w;
    uint32_t row_h = gui_files_window_row_h(win);
    uint32_t visible_index = item_index - gui_explorer_scroll;
    uint32_t y = content_y + 36 + visible_index * row_h;
    uint8_t selected = (item_index == gui_explorer_selected);
    uint8_t text_color = 0;

    gui_rect(content_x + 8, y - 5, content_w - 16, 19, selected ? 14 : 15);

    if (selected) {
        gui_rect_border(content_x + 8, y - 5, content_w - 16, 19, 0);
        gui_draw_text(content_x + 12, y, ">", 0);
    }

    gui_icon_draw(child, content_x + 32, y - 4, selected);
    gui_draw_text(content_x + 64, y, child->name, text_color);

    if (child->type == FS_DIR) {
        gui_draw_text(content_x + content_w - 90, y, "DIR", text_color);
    } else {
        gui_draw_text(content_x + content_w - 90, y, "FILE", text_color);
    }
}

static void gui_draw_file_explorer_window(desktop_window_t* win) {
    if (!win || !win->visible || win->app != DESKTOP_APP_FILES) {
        return;
    }

    uint32_t content_x = win->x + 24;
    uint32_t content_y = win->y + 46;
    uint32_t content_w = win->w > 48 ? win->w - 48 : win->w;
    uint32_t content_h = win->h > 76 ? win->h - 86 : win->h;

    char path[128];
    gui_build_node_path(gui_explorer_dir, path, 128);

    gui_draw_text(content_x, content_y, "Path:", 0);
    gui_draw_text(content_x + 52, content_y, path, 0);

    gui_panel(content_x, content_y + 24, content_w, content_h, 15, 0);

    uint32_t count = fs_count_children(gui_explorer_dir);

    if (count == 0) {
        gui_draw_text(content_x + 16, content_y + 48, "This directory is empty", 0);
        gui_draw_text(content_x + 16, win->y + win->h - 28, "Backspace up  Space menu  Q close", 8);
        return;
    }

    if (gui_explorer_selected >= count) {
        gui_explorer_selected = count - 1;
    }

    gui_files_window_adjust_scroll(win);

    uint32_t max_visible = gui_files_window_max_visible(win);

    if (max_visible > count) {
        max_visible = count;
    }

    for (uint32_t i = 0; i < max_visible; i++) {
        uint32_t item_index = gui_explorer_scroll + i;

        if (item_index >= count) {
            break;
        }

        gui_draw_file_explorer_window_row(win, item_index);
    }

    if (count > max_visible) {
        gui_draw_text(content_x + 16, win->y + win->h - 44, "More items above/below", 8);
    }

    gui_draw_text(content_x + 16, win->y + win->h - 28, "W/S move  Enter open  Backspace up  Space menu", 8);
}


static void gui_draw_desktop_window(desktop_window_t* win) {
    if (!win || !win->visible) {
        return;
    }

    gui_rect(win->x, win->y, win->w, win->h, 7);
    gui_rect_border(win->x, win->y, win->w, win->h, 0);

    gui_rect(win->x, win->y, win->w, 32, 9);
    gui_draw_text(win->x + 12, win->y + 12, win->title, 15);

    /*
       Botón cerrar decorativo.
    */
    gui_rect(win->x + win->w - 32, win->y + 8, 16, 16, 12);
    gui_rect_border(win->x + win->w - 32, win->y + 8, 16, 16, 15);

    if (win->app == DESKTOP_APP_FILES) {
        if (gui_screen == GUI_SCREEN_FILE_PREVIEW) {
            gui_draw_file_preview_window(win);
        } else if (gui_screen == GUI_SCREEN_TEXT_EDITOR) {
            gui_draw_text_editor_window(win);
        } else {
            gui_draw_file_explorer_window(win);
        }
    }

else if (win->app == DESKTOP_APP_TERMINAL) {
    gui_draw_terminal_window(win);
}

    else if (win->app == DESKTOP_APP_INFO) {
        gui_draw_text(win->x + 24, win->y + 58, "Bionic Kernel", 0);
        gui_draw_text(win->x + 24, win->y + 82, "1920x1080 framebuffer", 0);
        gui_draw_text(win->x + 24, win->y + 106, "RAMFS + GUI enabled", 0);
    }
}

static void gui_draw_context_menu(void) {
    const char* items[GUI_CONTEXT_ITEMS] = {
        "New file",
        "New folder",
        "Delete",
        "Rename",
        "Copy",
        "Move"
    };

    uint32_t x;
    uint32_t y;
    uint32_t w = 180;
    uint32_t h = 132;

    if (active_window.visible && active_window.app == DESKTOP_APP_FILES) {
        x = active_window.x + active_window.w - w - 32;
        y = active_window.y + 92;
    } else {
        x = 178;
        y = 70;
    }

    gui_panel(x, y, w, h, 7, 0);
    gui_rect(x, y, w, 22, 9);
    gui_draw_text(x + 10, y + 8, "Context", 15);

    for (uint32_t i = 0; i < GUI_CONTEXT_ITEMS; i++) {
        uint32_t item_y = y + 32 + i * 16;

        if (i == gui_context_selected) {
            gui_rect(x + 8, item_y - 3, w - 16, 14, 14);
            gui_draw_text(x + 14, item_y, ">", 0);
            gui_draw_text(x + 32, item_y, items[i], 0);
        } else {
            gui_draw_text(x + 32, item_y, items[i], 0);
        }
    }
}


static void gui_open_context_menu(void) {
    gui_screen = GUI_SCREEN_CONTEXT_MENU;
    gui_context_selected = 0;
    gui_draw_context_menu();
}

static void gui_context_menu_handle_key(char c) {
    if (c == 'w' || c == 'W') {
        if (gui_context_selected > 0) {
            gui_context_selected--;
        } else {
            gui_context_selected = GUI_CONTEXT_ITEMS - 1;
        }

        gui_draw_context_menu();
        return;
    }

    if (c == 's' || c == 'S') {
        gui_context_selected++;

        if (gui_context_selected >= GUI_CONTEXT_ITEMS) {
            gui_context_selected = 0;
        }

        gui_draw_context_menu();
        return;
    }

    if (c == '\n') {
        if (gui_context_selected == 0) {
            /*
               New file
            */
            gui_open_new_file_dialog();
            return;
        }

    if (gui_context_selected == 1) {
        gui_open_new_folder_dialog();
        return;
    }

        if (gui_context_selected == 2) {
        gui_delete_selected_item();
        return;
    }

        if (gui_context_selected == 3) {
            gui_open_rename_dialog();
            return;
    }

if (gui_context_selected == 4) {
    gui_open_copy_dialog();
    return;
}

if (gui_context_selected == 5) {
    gui_open_move_dialog();
    return;
}
    }

    if (c == 'q' || c == 'Q' || c == '\b') {
        if (active_window.visible && active_window.app == DESKTOP_APP_FILES) {
            gui_screen = GUI_SCREEN_DESKTOP;
            gui_draw_desktop_window(&active_window);
        } else {
            gui_screen = GUI_SCREEN_FILES;
            gui_draw_file_explorer();
            gui_statusbar("Context menu closed");
        }
        return;
    }
}

static void gui_editor_clear_buffer(void) {
    for (uint32_t i = 0; i < 512; i++) {
        gui_editor_buffer[i] = 0;
    }

    gui_editor_pos = 0;
}

static void gui_editor_load_file(fs_node_t* file) {
    gui_editor_clear_buffer();

    if (!file || file->type != FS_FILE) {
        return;
    }

    uint32_t i = 0;

    while (file->content[i] && i < 511) {
        gui_editor_buffer[i] = file->content[i];
        i++;
    }

    gui_editor_buffer[i] = 0;
    gui_editor_pos = i;
}

static void gui_editor_add_char(char c) {
    if (gui_editor_pos >= 511) {
        return;
    }

    gui_editor_buffer[gui_editor_pos++] = c;
    gui_editor_buffer[gui_editor_pos] = 0;
}

static void gui_editor_backspace(void) {
    if (gui_editor_pos == 0) {
        return;
    }

    gui_editor_pos--;
    gui_editor_buffer[gui_editor_pos] = 0;
}

static int gui_editor_is_allowed_char(char c) {
    if (c >= 'a' && c <= 'z') return 1;
    if (c >= 'A' && c <= 'Z') return 1;
    if (c >= '0' && c <= '9') return 1;

    if (c == ' ') return 1;
    if (c == '.') return 1;
    if (c == ',') return 1;
    if (c == ':') return 1;
    if (c == ';') return 1;
    if (c == '-') return 1;
    if (c == '_') return 1;
    if (c == '/') return 1;
    if (c == '!') return 1;
    if (c == '?') return 1;
    if (c == '(') return 1;
    if (c == ')') return 1;
    if (c == '[') return 1;
    if (c == ']') return 1;

    return 0;
}

static void gui_editor_save(void) {
    if (!gui_editor_file || gui_editor_file->type != FS_FILE) {
        return;
    }

    for (uint32_t i = 0; i < 512; i++) {
        gui_editor_file->content[i] = 0;
    }

    uint32_t i = 0;

    while (gui_editor_buffer[i] && i < 511) {
        gui_editor_file->content[i] = gui_editor_buffer[i];
        i++;
    }

    gui_editor_file->content[i] = 0;
    gui_editor_file->size = i;
}

static void gui_draw_text_editor_window(desktop_window_t* win) {
    if (!win || !win->visible || win->app != DESKTOP_APP_FILES) {
        return;
    }

    uint32_t content_x = win->x + 24;
    uint32_t content_y = win->y + 46;
    uint32_t content_w = win->w > 48 ? win->w - 48 : win->w;
    uint32_t editor_y = content_y + 54;
    uint32_t footer_y = win->y + win->h - 28;
    uint32_t content_h = footer_y > editor_y + 14 ? footer_y - editor_y - 14 : 60;

    gui_draw_text(content_x, content_y, "Text editor", 0);

    if (!gui_editor_file) {
        gui_draw_text(content_x + 16, content_y + 42, "No file selected", 0);
        gui_draw_text(content_x + 16, win->y + win->h - 28, "Backspace return", 8);
        return;
    }

    gui_draw_text(content_x, content_y + 24, "File:", 0);
    gui_draw_text(content_x + 52, content_y + 24, gui_editor_file->name, 0);

    gui_panel(content_x, editor_y, content_w, content_h, 15, 0);

    gui_draw_wrapped_text(
        content_x + 16,
        editor_y + 20,
        (content_w > 32 ? (content_w - 32) / 8 : 1),
        (content_h > 20 ? (content_h - 20) / 10 : 1),
        gui_editor_buffer,
        0
    );

    uint32_t chars_per_line = (content_w > 32 ? (content_w - 32) / 8 : 1);
    if (chars_per_line == 0) {
        chars_per_line = 1;
    }

    uint32_t cursor_x = content_x + 16 + ((gui_editor_pos % chars_per_line) * 8);
    uint32_t cursor_y = editor_y + 20 + ((gui_editor_pos / chars_per_line) * 10);

    if (cursor_y < editor_y + content_h - 8) {
        gui_rect(cursor_x, cursor_y, 6, 8, 8);
    }

    gui_draw_text(content_x + 16, win->y + win->h - 28, "Type text  Tab save  Backspace delete", 8);
}

static void gui_draw_text_editor(void) {
    if (active_window.visible && active_window.app == DESKTOP_APP_FILES) {
        gui_draw_desktop_window(&active_window);
        return;
    }

    gui_clear(GUI_COLOR_BG);

    gui_rect(0, 0, VGA_WIDTH, 24, GUI_COLOR_TOPBAR);
    gui_draw_text(8, 8, "Bionic Text Editor", GUI_COLOR_TEXT_INV);

    gui_window(12, 30, 296, 150, "Editor");

    if (!gui_editor_file) {
        gui_label(28, 58, "No file selected", GUI_COLOR_TEXT);
        gui_statusbar("Q return");
        return;
    }

    gui_label(26, 56, "File:", GUI_COLOR_TEXT);
    gui_label(74, 56, gui_editor_file->name, GUI_COLOR_TEXT);

    gui_panel(22, 72, 276, 88, 15, 0);

    gui_draw_wrapped_text(30, 82, 32, 7, gui_editor_buffer, 0);
    gui_rect(30 + ((gui_editor_pos % 32) * 8), 82 + ((gui_editor_pos / 32) * 10), 6, 8, 8);

    gui_statusbar("Type text  Tab save  Backspace delete");
}

static void gui_open_text_editor(fs_node_t* file) {
    if (!file || file->type != FS_FILE) {
        return;
    }

    gui_editor_file = file;
    gui_editor_load_file(file);

    gui_screen = GUI_SCREEN_TEXT_EDITOR;
    gui_draw_text_editor();
}


static void gui_redraw_text_editor_content_area(void) {
    if (!(active_window.visible && active_window.app == DESKTOP_APP_FILES)) {
        gui_draw_text_editor();
        return;
    }

    uint32_t content_x = active_window.x + 24;
    uint32_t content_y = active_window.y + 46;
    uint32_t content_w = active_window.w > 48 ? active_window.w - 48 : active_window.w;
    uint32_t editor_y = content_y + 54;
    uint32_t footer_y = active_window.y + active_window.h - 28;
    uint32_t content_h = footer_y > editor_y + 14 ? footer_y - editor_y - 14 : 60;

    gui_panel(content_x, editor_y, content_w, content_h, 15, 0);

    gui_draw_wrapped_text(
        content_x + 16,
        editor_y + 20,
        (content_w > 32 ? (content_w - 32) / 8 : 1),
        (content_h > 20 ? (content_h - 20) / 10 : 1),
        gui_editor_buffer,
        0
    );

    uint32_t chars_per_line = (content_w > 32 ? (content_w - 32) / 8 : 1);
    if (chars_per_line == 0) {
        chars_per_line = 1;
    }

    uint32_t cursor_x = content_x + 16 + ((gui_editor_pos % chars_per_line) * 8);
    uint32_t cursor_y = editor_y + 20 + ((gui_editor_pos / chars_per_line) * 10);

    if (cursor_y < editor_y + content_h - 8) {
        gui_rect(cursor_x, cursor_y, 6, 8, 8);
    }
}

static void gui_text_editor_handle_key(char c) {
    if (c == '\t') {
        gui_editor_save();

        gui_preview_file = gui_editor_file;
        gui_screen = GUI_SCREEN_FILE_PREVIEW;
        gui_draw_file_preview();
        return;
    }

    if (c == '\b') {
        gui_editor_backspace();
        gui_redraw_text_editor_content_area();
        return;
    }

    if (c == '\n') {
        gui_editor_add_char('\n');
        gui_redraw_text_editor_content_area();
        return;
    }

    if (gui_editor_is_allowed_char(c)) {
        gui_editor_add_char(c);
        gui_redraw_text_editor_content_area();
        return;
    }
}

static int gui_rects_intersect(
    int ax, int ay, int aw, int ah,
    int bx, int by, int bw, int bh
) {
    if (ax + aw <= bx) return 0;
    if (bx + bw <= ax) return 0;
    if (ay + ah <= by) return 0;
    if (by + bh <= ay) return 0;

    return 1;
}

static void gui_redraw_desktop_icons_in_region(int rx, int ry, int rw, int rh) {
    /*
       Zonas aproximadas de los iconos del escritorio.
       Deben coincidir con las posiciones que usas en gui_redraw_desktop().
    */

    if (gui_rects_intersect(rx, ry, rw, rh, 50, 80, 140, 80)) {
        gui_draw_desktop_icon_by_index(0, desktop_selected_icon == 0);
    }

    if (gui_rects_intersect(rx, ry, rw, rh, 50, 180, 140, 80)) {
        gui_draw_desktop_icon_by_index(1, desktop_selected_icon == 1);
    }

    if (gui_rects_intersect(rx, ry, rw, rh, 50, 280, 140, 80)) {
        gui_draw_desktop_icon_by_index(2, desktop_selected_icon == 2);
    }
}

static void gui_redraw_taskbar_if_needed(int rx, int ry, int rw, int rh) {
    uint32_t screen_h = gui_get_height();
    uint32_t taskbar_y = screen_h - 48;

    if (gui_rects_intersect(rx, ry, rw, rh, 0, taskbar_y, gui_get_width(), 48)) {
        gui_draw_taskbar();
    }
}

static void gui_restore_desktop_region(int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) {
        return;
    }

    /*
       Restaurar fondo solo en esa región.
    */
    gui_draw_desktop_background_region(x, y, w, h);

    /*
       Si había iconos debajo, redibujarlos.
    */
    gui_redraw_desktop_icons_in_region(x, y, w, h);

    /*
       Si la región toca la barra de tareas, redibujarla.
    */
    gui_redraw_taskbar_if_needed(x, y, w, h);
}


static void gui_close_active_window(void) {
    if (!active_window.visible) {
        return;
    }

    int old_x = active_window.x;
    int old_y = active_window.y;
    int old_w = active_window.w;
    int old_h = active_window.h;

    active_window.visible = 0;
    active_window.app = DESKTOP_APP_NONE;

    gui_restore_desktop_region(old_x - 2, old_y - 2, old_w + 4, old_h + 4);
}

static void gui_move_active_window(int dx, int dy) {
    if (!active_window.visible) {
        return;
    }

    int old_x = active_window.x;
    int old_y = active_window.y;
    int old_w = active_window.w;
    int old_h = active_window.h;

    int new_x = active_window.x + dx;
    int new_y = active_window.y + dy;

    /*
       Limites simples para que no se vaya totalmente fuera.
    */
    if (new_x < 0) {
        new_x = 0;
    }

    if (new_y < 42) {
        new_y = 42;
    }

    if (new_x + active_window.w > (int)gui_get_width()) {
        new_x = gui_get_width() - active_window.w;
    }

    if (new_y + active_window.h > (int)gui_get_height() - 48) {
        new_y = gui_get_height() - 48 - active_window.h;
    }

    /*
       Si no se ha movido, no hacemos nada.
    */
    if (new_x == active_window.x && new_y == active_window.y) {
        return;
    }

    /*
       Restauramos la zona vieja de la ventana.
       Añadimos unos píxeles extra por bordes.
    */
    gui_restore_desktop_region(old_x - 2, old_y - 2, old_w + 4, old_h + 4);

    /*
       Actualizamos posición.
    */
    active_window.x = new_x;
    active_window.y = new_y;

    /*
       Dibujamos la ventana en la nueva posición.
    */
    gui_draw_desktop_window(&active_window);
}


static void gui_files_window_handle_key(char c) {
    uint32_t count = fs_count_children(gui_explorer_dir);

    if (c == 'w' || c == 'W') {
        if (count > 0) {
            uint32_t old_selected = gui_explorer_selected;
            uint32_t old_scroll = gui_explorer_scroll;

            if (gui_explorer_selected > 0) {
                gui_explorer_selected--;
            } else {
                gui_explorer_selected = count - 1;
            }

            gui_files_window_adjust_scroll(&active_window);

            if (old_scroll != gui_explorer_scroll) {
                gui_draw_desktop_window(&active_window);
            } else {
                gui_draw_file_explorer_window_row(&active_window, old_selected);
                gui_draw_file_explorer_window_row(&active_window, gui_explorer_selected);
            }
        }

        return;
    }

    if (c == 's' || c == 'S') {
        if (count > 0) {
            uint32_t old_selected = gui_explorer_selected;
            uint32_t old_scroll = gui_explorer_scroll;

            gui_explorer_selected++;

            if (gui_explorer_selected >= count) {
                gui_explorer_selected = 0;
            }

            gui_files_window_adjust_scroll(&active_window);

            if (old_scroll != gui_explorer_scroll) {
                gui_draw_desktop_window(&active_window);
            } else {
                gui_draw_file_explorer_window_row(&active_window, old_selected);
                gui_draw_file_explorer_window_row(&active_window, gui_explorer_selected);
            }
        }

        return;
    }

    if (c == '\n') {
        fs_node_t* selected = fs_get_child_at(gui_explorer_dir, gui_explorer_selected);

        if (!selected) {
            gui_draw_desktop_window(&active_window);
            return;
        }

        if (selected->type == FS_DIR) {
            gui_explorer_dir = selected;
            gui_explorer_selected = 0;
            gui_explorer_scroll = 0;
            gui_draw_desktop_window(&active_window);
        } else {
            gui_open_file_preview(selected);
        }

        return;
    }

    if (c == '\b') {
        if (gui_explorer_dir && gui_explorer_dir->parent && gui_explorer_dir != fs_get_root()) {
            gui_explorer_dir = gui_explorer_dir->parent;
            gui_explorer_selected = 0;
            gui_explorer_scroll = 0;
        }

        gui_draw_desktop_window(&active_window);
        return;
    }

    if (c == ' ') {
        gui_open_context_menu();
        return;
    }
}

static void gui_term_clear_buffer(void) {
    for (uint32_t i = 0; i < GUI_TERM_MAX_LINES; i++) {
        for (uint32_t j = 0; j < GUI_TERM_LINE_LEN; j++) {
            gui_term_lines[i][j] = 0;
        }
    }

    gui_term_line_count = 0;
}

static void gui_term_clear_input(void) {
    for (uint32_t i = 0; i < GUI_TERM_INPUT_LEN; i++) {
        gui_term_input[i] = 0;
    }

    gui_term_input_pos = 0;
}

static void gui_term_add_line(const char* text) {
    if (!text) {
        return;
    }

    if (gui_term_line_count >= GUI_TERM_MAX_LINES) {
        for (uint32_t i = 1; i < GUI_TERM_MAX_LINES; i++) {
            for (uint32_t j = 0; j < GUI_TERM_LINE_LEN; j++) {
                gui_term_lines[i - 1][j] = gui_term_lines[i][j];
            }
        }

        gui_term_line_count = GUI_TERM_MAX_LINES - 1;
    }

    uint32_t line = gui_term_line_count;

    for (uint32_t j = 0; j < GUI_TERM_LINE_LEN; j++) {
        gui_term_lines[line][j] = 0;
    }

    uint32_t i = 0;
    while (text[i] && i < GUI_TERM_LINE_LEN - 1) {
        gui_term_lines[line][i] = text[i];
        i++;
    }

    gui_term_line_count++;
}

static void gui_term_add_input_echo(void) {
    char echo[GUI_TERM_LINE_LEN];

    for (uint32_t i = 0; i < GUI_TERM_LINE_LEN; i++) {
        echo[i] = 0;
    }

    echo[0] = '>';
    echo[1] = ' ';

    uint32_t pos = 2;
    uint32_t i = 0;

    while (gui_term_input[i] && pos < GUI_TERM_LINE_LEN - 1) {
        echo[pos++] = gui_term_input[i++];
    }

    echo[pos] = 0;

    gui_term_add_line(echo);
}

static void gui_term_input_add_char(char c) {
    if (gui_term_input_pos >= GUI_TERM_INPUT_LEN - 1) {
        return;
    }

    gui_term_input[gui_term_input_pos++] = c;
    gui_term_input[gui_term_input_pos] = 0;
}

static void gui_term_input_backspace(void) {
    if (gui_term_input_pos == 0) {
        return;
    }

    gui_term_input_pos--;
    gui_term_input[gui_term_input_pos] = 0;
}

static int gui_term_streq(const char* a, const char* b) {
    uint32_t i = 0;

    while (a[i] && b[i]) {
        if (a[i] != b[i]) {
            return 0;
        }

        i++;
    }

    return a[i] == b[i];
}

static int gui_term_starts_with(const char* text, const char* prefix) {
    uint32_t i = 0;

    while (prefix[i]) {
        if (text[i] != prefix[i]) {
            return 0;
        }

        i++;
    }

    return 1;
}

static void gui_terminal_redraw_input_line(desktop_window_t* win) {
    if (!win || !win->visible) {
        return;
    }

    uint32_t content_x = win->x + 18;
    uint32_t content_y = win->y + 48;
    uint32_t content_w = win->w - 36;
    uint32_t content_h = win->h - 76;

    uint32_t input_y = content_y + content_h - 22;

    /*
       Limpiamos solo la zona donde se escribe el comando.
    */
    gui_rect(content_x + 4, input_y - 4, content_w - 8, 18, 0);

    /*
       Redibujamos prompt + input.
    */
    gui_draw_text(content_x + 10, input_y, ">", 15);
    gui_draw_text(content_x + 28, input_y, gui_term_input, 15);

    /*
       Cursor.
    */
    gui_rect(content_x + 28 + gui_term_input_pos * 8, input_y, 7, 9, 15);
}

static void gui_draw_terminal_window(desktop_window_t* win) {
    if (!win || !win->visible) {
        return;
    }

    uint32_t content_x = win->x + 18;
    uint32_t content_y = win->y + 48;
    uint32_t content_w = win->w - 36;
    uint32_t content_h = win->h - 76;

    gui_rect(content_x, content_y, content_w, content_h, 0);
    gui_rect_border(content_x, content_y, content_w, content_h, 15);

    uint32_t y = content_y + 12;

    uint32_t max_lines = (content_h - 36) / 12;
    uint32_t start = 0;

    if (gui_term_line_count > max_lines) {
        start = gui_term_line_count - max_lines;
    }

    for (uint32_t i = start; i < gui_term_line_count; i++) {
        gui_draw_text(content_x + 10, y, gui_term_lines[i], 10);
        y += 12;
    }

    /*
       Línea de input.
    */
    uint32_t input_y = content_y + content_h - 22;

    gui_rect(content_x + 4, input_y - 4, content_w - 8, 18, 0);
    gui_draw_text(content_x + 10, input_y, ">", 15);
    gui_draw_text(content_x + 28, input_y, gui_term_input, 15);

    /*
       Cursor.
    */
    gui_rect(content_x + 28 + gui_term_input_pos * 8, input_y, 7, 9, 15);
}

static void gui_terminal_execute_command(void) {
    if (gui_term_input[0] == '\0') {
        return;
    }

    gui_term_add_input_echo();

    if (gui_term_streq(gui_term_input, "help")) {
        gui_term_add_line("Commands:");
        gui_term_add_line("help clear pwd ls cd mkdir touch cat write rm tree exit");
    }

    else if (gui_term_streq(gui_term_input, "clear")) {
        gui_term_clear_buffer();
    }

    else if (gui_term_streq(gui_term_input, "pwd")) {
        char path[128];
        gui_build_node_path(fs_get_current(), path, 128);
        gui_term_add_line(path);
    }

    else if (gui_term_starts_with(gui_term_input, "cd ")) {
        const char* path = gui_term_input + 3;

        if (fs_cd(path) == 0) {
            gui_term_add_line("OK");
        } else {
            gui_term_add_line("cd failed");
        }
    }

    else if (gui_term_streq(gui_term_input, "ls")) {
        fs_node_t* dir = fs_get_current();

        if (!dir || dir->type != FS_DIR) {
            gui_term_add_line("Not a directory");
        } else {
            uint32_t count = fs_count_children(dir);

            if (count == 0) {
                gui_term_add_line("[empty]");
            }

            for (uint32_t i = 0; i < count; i++) {
                fs_node_t* child = fs_get_child_at(dir, i);

                if (!child) {
                    continue;
                }

                char line[GUI_TERM_LINE_LEN];

                for (uint32_t j = 0; j < GUI_TERM_LINE_LEN; j++) {
                    line[j] = 0;
                }

                uint32_t pos = 0;

                if (child->type == FS_DIR) {
                    line[pos++] = '[';
                    line[pos++] = 'D';
                    line[pos++] = 'I';
                    line[pos++] = 'R';
                    line[pos++] = ']';
                    line[pos++] = ' ';
                } else {
                    line[pos++] = '[';
                    line[pos++] = 'F';
                    line[pos++] = 'I';
                    line[pos++] = 'L';
                    line[pos++] = 'E';
                    line[pos++] = ']';
                    line[pos++] = ' ';
                }

                uint32_t k = 0;
                while (child->name[k] && pos < GUI_TERM_LINE_LEN - 1) {
                    line[pos++] = child->name[k++];
                }

                line[pos] = 0;
                gui_term_add_line(line);
            }
        }
    }

    else if (gui_term_starts_with(gui_term_input, "mkdir ")) {
        const char* path = gui_term_input + 6;

        if (fs_mkdir(path) == 0) {
            gui_term_add_line("Directory created");
        } else {
            gui_term_add_line("mkdir failed");
        }
    }

    else if (gui_term_starts_with(gui_term_input, "touch ")) {
        const char* path = gui_term_input + 6;

        if (fs_touch(path) == 0) {
            gui_term_add_line("File created");
        } else {
            gui_term_add_line("touch failed");
        }
    }

    else if (gui_term_starts_with(gui_term_input, "cat ")) {
        const char* path = gui_term_input + 4;
        fs_node_t* node = fs_get_node(path);

        if (!node || node->type != FS_FILE) {
            gui_term_add_line("cat failed");
        } else {
            if (node->content[0] == '\0') {
                gui_term_add_line("[empty file]");
            } else {
                gui_term_add_line(node->content);
            }
        }
    }

    else if (gui_term_starts_with(gui_term_input, "write ")) {
        const char* rest = gui_term_input + 6;

        uint32_t i = 0;
        while (rest[i] && rest[i] != ' ') {
            i++;
        }

        if (rest[i] == '\0') {
            gui_term_add_line("Usage: write file text");
        } else {
            char path[64];

            for (uint32_t j = 0; j < 64; j++) {
                path[j] = 0;
            }

            for (uint32_t j = 0; j < i && j < 63; j++) {
                path[j] = rest[j];
            }

            const char* text = rest + i + 1;

            if (fs_write(path, text) == 0) {
                gui_term_add_line("Written");
            } else {
                gui_term_add_line("write failed");
            }
        }
    }

    else if (gui_term_starts_with(gui_term_input, "rm ")) {
        const char* path = gui_term_input + 3;

        if (fs_rm(path) == 0) {
            gui_term_add_line("Removed");
        } else {
            gui_term_add_line("rm failed");
        }
    }

    else if (gui_term_streq(gui_term_input, "tree")) {
        gui_term_add_line("tree not available in GUI terminal yet");
    }

    else if (gui_term_streq(gui_term_input, "exit")) {
        active_window.visible = 0;
        active_window.app = DESKTOP_APP_NONE;
        gui_redraw_desktop();
        gui_term_clear_input();
        return;
    }

    else {
        gui_term_add_line("Unknown command");
    }

    gui_term_clear_input();
}

static int gui_terminal_is_allowed_char(char c) {
    if (c >= 'a' && c <= 'z') return 1;
    if (c >= 'A' && c <= 'Z') return 1;
    if (c >= '0' && c <= '9') return 1;

    if (c == ' ') return 1;
    if (c == '.') return 1;
    if (c == ',') return 1;
    if (c == ':') return 1;
    if (c == ';') return 1;
    if (c == '-') return 1;
    if (c == '_') return 1;
    if (c == '/') return 1;
    if (c == '!') return 1;
    if (c == '?') return 1;
    if (c == '[') return 1;
    if (c == ']') return 1;

    return 0;
}

static void gui_terminal_handle_key(char c) {
    if (c == '\n') {
        /*
           Al ejecutar un comando sí redibujamos la terminal,
           porque cambian las líneas de salida.
        */
        gui_terminal_execute_command();
        gui_draw_desktop_window(&active_window);
        return;
    }

    if (c == '\b') {
        gui_term_input_backspace();

        /*
           Solo redibuja la línea de escritura.
        */
        gui_terminal_redraw_input_line(&active_window);
        return;
    }

    if (gui_terminal_is_allowed_char(c)) {
        gui_term_input_add_char(c);

        /*
           Solo redibuja la línea de escritura.
        */
        gui_terminal_redraw_input_line(&active_window);
        return;
    }
}

void gui_handle_key(char c) {
    /*
       Pantallas especiales que tienen su propio control
    */

    if (gui_screen == GUI_SCREEN_CONTEXT_MENU) {
        gui_context_menu_handle_key(c);
        return;
    }

    if (gui_screen == GUI_SCREEN_TEXT_EDITOR) {
        gui_text_editor_handle_key(c);
        return;
    }

    if (gui_screen == GUI_SCREEN_NEW_FILE) {
        gui_new_file_handle_key(c);
        return;
    }

    if (gui_screen == GUI_SCREEN_NEW_FOLDER) {
        gui_new_folder_handle_key(c);
        return;
    }

    if (gui_screen == GUI_SCREEN_RENAME) {
        gui_rename_handle_key(c);
        return;
    }

    if (gui_screen == GUI_SCREEN_COPY) {
        gui_copy_handle_key(c);
        return;
    }

    if (gui_screen == GUI_SCREEN_MOVE) {
        gui_move_handle_key(c);
        return;
    }

    if (gui_screen == GUI_SCREEN_FILE_PREVIEW) {
        gui_preview_handle_key(c);
        return;
    }

    if (active_window.visible && active_window.app == DESKTOP_APP_TERMINAL) {
    gui_terminal_handle_key(c);
    return;
}

    if (gui_screen == GUI_SCREEN_FILES) {
        if (active_window.visible && active_window.app == DESKTOP_APP_FILES) {
            gui_screen = GUI_SCREEN_DESKTOP;
            gui_files_window_handle_key(c);
        } else {
            gui_files_handle_key(c);
        }
        return;
    }

    /*
       Pantallas simples: Terminal / Info
       De momento solo permiten volver al escritorio.
    */

    if (gui_screen == GUI_SCREEN_TERMINAL || gui_screen == GUI_SCREEN_INFO) {
        if (c == 'q' || c == 'Q' || c == '\b') {
            gui_screen = GUI_SCREEN_DESKTOP;
            gui_redraw_desktop();
        }

        return;
    }

    /*
       Si hay una app de Files abierta en una ventana, W/S/Enter/Backspace
       controlan el explorador dentro de esa ventana. I/J/K/L siguen moviendo
       la ventana y Q la cierra.
    */
    if (active_window.visible && active_window.app == DESKTOP_APP_FILES) {
        if (c == 'j' || c == 'J') {
            gui_move_active_window(-20, 0);
            return;
        }

        if (c == 'l' || c == 'L') {
            gui_move_active_window(20, 0);
            return;
        }

        if (c == 'i' || c == 'I') {
            gui_move_active_window(0, -20);
            return;
        }

        if (c == 'k' || c == 'K') {
            gui_move_active_window(0, 20);
            return;
        }

        if (c == 'q' || c == 'Q') {
            gui_close_active_window();
            return;
        }

        gui_files_window_handle_key(c);
        return;
    }

    /*
       Escritorio principal
       A/D o W/S cambian el icono seleccionado.
       Enter abre la app seleccionada.
    */

if (c == 'a' || c == 'A' || c == 'w' || c == 'W') {
    uint32_t old_icon = desktop_selected_icon;

    if (desktop_selected_icon > 0) {
        desktop_selected_icon--;
    } else {
        desktop_selected_icon = DESKTOP_ICON_COUNT - 1;
    }

    gui_update_desktop_selection(old_icon, desktop_selected_icon);
}

else if (c == 'd' || c == 'D' || c == 's' || c == 'S') {
    uint32_t old_icon = desktop_selected_icon;

    desktop_selected_icon++;

    if (desktop_selected_icon >= DESKTOP_ICON_COUNT) {
        desktop_selected_icon = 0;
    }

    gui_update_desktop_selection(old_icon, desktop_selected_icon);

}

else if (c == 'j' || c == 'J') {
    gui_move_active_window(-20, 0);
}

else if (c == 'l' || c == 'L') {
    gui_move_active_window(20, 0);
}

else if (c == 'i' || c == 'I') {
    gui_move_active_window(0, -20);
}

else if (c == 'k' || c == 'K') {
    gui_move_active_window(0, 20);
}


else if (c == 'q' || c == 'Q') {
    gui_close_active_window();
}

else if (c == '\n') {
    gui_desktop_open_selected_app();
}

}

static void gui_save_cursor_background(int32_t x, int32_t y) {
    for (uint32_t cy = 0; cy < GUI_CURSOR_H; cy++) {
        for (uint32_t cx = 0; cx < GUI_CURSOR_W; cx++) {
            int32_t px = x + (int32_t)cx;
            int32_t py = y + (int32_t)cy;

            if (px >= 0 && py >= 0 &&
                px < (int32_t)gui_get_width() &&
                py < (int32_t)gui_get_height()) {
                gui_cursor_backup[cy * GUI_CURSOR_W + cx] =
                    gui_read_raw_pixel((uint32_t)px, (uint32_t)py);
            } else {
                gui_cursor_backup[cy * GUI_CURSOR_W + cx] = 0;
            }
        }
    }

    gui_cursor_backup_valid = 1;
}

static void gui_restore_cursor_background(int32_t x, int32_t y) {
    if (!gui_cursor_backup_valid) {
        return;
    }

    for (uint32_t cy = 0; cy < GUI_CURSOR_H; cy++) {
        for (uint32_t cx = 0; cx < GUI_CURSOR_W; cx++) {
            int32_t px = x + (int32_t)cx;
            int32_t py = y + (int32_t)cy;

            if (px >= 0 && py >= 0 &&
                px < (int32_t)gui_get_width() &&
                py < (int32_t)gui_get_height()) {
                gui_write_raw_pixel(
                    (uint32_t)px,
                    (uint32_t)py,
                    gui_cursor_backup[cy * GUI_CURSOR_W + cx]
                );
            }
        }
    }
}

static void gui_xor_pixel(int32_t x, int32_t y) {
    if (x < 0 || y < 0) {
        return;
    }

    if (x >= (int32_t)gui_get_width() || y >= (int32_t)gui_get_height()) {
        return;
    }

    uint32_t color = gui_read_raw_pixel((uint32_t)x, (uint32_t)y);

    /*
       Invierte RGB pero mantiene alpha si existe.
       En framebuffer 32-bit se ve como línea contrastada.
    */
    color ^= 0x00FFFFFF;

    gui_write_raw_pixel((uint32_t)x, (uint32_t)y, color);
}

static void gui_draw_drag_outline(int32_t x, int32_t y, int32_t w, int32_t h) {
    /*
       Contorno punteado. Al llamarlo dos veces sobre la misma posición,
       se borra automáticamente por XOR.
    */

    for (int32_t xx = 0; xx < w; xx += 6) {
        gui_xor_pixel(x + xx, y);
        gui_xor_pixel(x + xx + 1, y);

        gui_xor_pixel(x + xx, y + h - 1);
        gui_xor_pixel(x + xx + 1, y + h - 1);
    }

    for (int32_t yy = 0; yy < h; yy += 6) {
        gui_xor_pixel(x, y + yy);
        gui_xor_pixel(x, y + yy + 1);

        gui_xor_pixel(x + w - 1, y + yy);
        gui_xor_pixel(x + w - 1, y + yy + 1);
    }
}

static void gui_clamp_window_position(int32_t* x, int32_t* y, int32_t w, int32_t h) {
    if (*x < 0) {
        *x = 0;
    }

    if (*y < 42) {
        *y = 42;
    }

    if (*x + w > (int32_t)gui_get_width()) {
        *x = (int32_t)gui_get_width() - w;
    }

    if (*y + h > (int32_t)gui_get_height() - 48) {
        *y = (int32_t)gui_get_height() - 48 - h;
    }

    if (*x < 0) {
        *x = 0;
    }

    if (*y < 42) {
        *y = 42;
    }
}

static void gui_mouse_left_pressed(int32_t x, int32_t y) {
    gui_mouse_click_counter++;

/*
   Si la ventana Files está abierta y el click cae dentro del área de lista,
   seleccionamos o abrimos con doble click.
*/
if (active_window.visible && active_window.app == DESKTOP_APP_FILES) {
    int item = gui_files_item_at_mouse(x, y);

    if (item >= 0) {
        gui_files_mouse_left_click(x, y);
        return;
    }
}
    /*
       Si hay una ventana visible y haces click en su barra superior,
       empezamos drag con outline.
    */
    if (active_window.visible) {
        if (gui_point_in_rect(x, y, active_window.x, active_window.y, active_window.w, 32)) {
            gui_dragging_window = 1;

            gui_drag_mouse_start_x = x;
            gui_drag_mouse_start_y = y;

            gui_drag_window_start_x = active_window.x;
            gui_drag_window_start_y = active_window.y;

            gui_drag_outline_x = active_window.x;
            gui_drag_outline_y = active_window.y;

            gui_draw_drag_outline(
                gui_drag_outline_x,
                gui_drag_outline_y,
                active_window.w,
                active_window.h
            );

            gui_drag_outline_visible = 1;
            return;
        }
    }

    /*
       Click en iconos del escritorio.
    */
    int icon = gui_desktop_icon_at(x, y);

    if (icon >= 0) {
        uint32_t old_icon = desktop_selected_icon;
        desktop_selected_icon = (uint32_t)icon;

        if (old_icon != desktop_selected_icon) {
            gui_update_desktop_selection(old_icon, desktop_selected_icon);
        }

        gui_desktop_open_selected_app();
        return;
    }
}

static void gui_mouse_right_pressed(int32_t x, int32_t y) {
    if (
        active_window.visible &&
        active_window.app == DESKTOP_APP_FILES &&
        (gui_screen == GUI_SCREEN_FILES || gui_screen == GUI_SCREEN_DESKTOP)
    ) {
        int index = gui_files_item_at_mouse(x, y);

        if (index >= 0) {
            gui_explorer_selected = (uint32_t)index;
            gui_draw_desktop_window(&active_window);
        }

        gui_open_context_menu();
        return;
    }
}

static void gui_mouse_left_released(int32_t x, int32_t y) {
    (void)x;
    (void)y;

    if (!gui_dragging_window) {
        return;
    }

    /*
       Borrar el outline final.
    */
    if (gui_drag_outline_visible) {
        gui_draw_drag_outline(
            gui_drag_outline_x,
            gui_drag_outline_y,
            active_window.w,
            active_window.h
        );

        gui_drag_outline_visible = 0;
    }

    /*
       Guardamos zona vieja de la ventana real.
    */
    int old_x = active_window.x;
    int old_y = active_window.y;
    int old_w = active_window.w;
    int old_h = active_window.h;

    /*
       Movemos la ventana real a la posición final.
    */
    active_window.x = gui_drag_outline_x;
    active_window.y = gui_drag_outline_y;

    /*
       Restauramos zona antigua y dibujamos una sola vez.
    */
    gui_restore_desktop_region(old_x - 2, old_y - 2, old_w + 4, old_h + 4);
    gui_draw_desktop_window(&active_window);

    gui_dragging_window = 0;
}

static void gui_mouse_process_buttons_and_drag(int32_t x, int32_t y, uint8_t left, uint8_t right) {
    if (left && !gui_mouse_left_prev) {
        gui_mouse_left_pressed(x, y);
    }

    if (right && !gui_mouse_right_prev) {
        gui_mouse_right_pressed(x, y);
    }

    if (left && gui_dragging_window) {
        int32_t new_x = gui_drag_window_start_x + (x - gui_drag_mouse_start_x);
        int32_t new_y = gui_drag_window_start_y + (y - gui_drag_mouse_start_y);

        gui_clamp_window_position(&new_x, &new_y, active_window.w, active_window.h);

        if (new_x != gui_drag_outline_x || new_y != gui_drag_outline_y) {
            if (gui_drag_outline_visible) {
                gui_draw_drag_outline(
                    gui_drag_outline_x,
                    gui_drag_outline_y,
                    active_window.w,
                    active_window.h
                );
            }

            gui_drag_outline_x = new_x;
            gui_drag_outline_y = new_y;

            gui_draw_drag_outline(
                gui_drag_outline_x,
                gui_drag_outline_y,
                active_window.w,
                active_window.h
            );

            gui_drag_outline_visible = 1;
        }
    }

    if (!left && gui_mouse_left_prev) {
        gui_mouse_left_released(x, y);
    }

    (void)right;
}

/*
static void gui_mouse_process_buttons_and_drag(int32_t x, int32_t y, uint8_t left, uint8_t right) {

    if (left && !gui_mouse_left_prev) {
        gui_mouse_left_pressed(x, y);
    }

    if (right && !gui_mouse_right_prev) {
    gui_mouse_right_pressed(x, y);
}

if (right && !gui_mouse_right_prev) {
if (
    active_window.visible &&
    active_window.app == DESKTOP_APP_FILES &&
    gui_screen == GUI_SCREEN_FILES
) {
    int item = gui_files_item_at_mouse(x, y);

    if (item >= 0) {
        gui_files_mouse_left_click(x, y);
        return;
    }
}
}



    if (left && gui_dragging_window) {
        int32_t new_x = gui_drag_window_start_x + (x - gui_drag_mouse_start_x);
        int32_t new_y = gui_drag_window_start_y + (y - gui_drag_mouse_start_y);

        gui_clamp_window_position(&new_x, &new_y, active_window.w, active_window.h);

        if (new_x != gui_drag_outline_x || new_y != gui_drag_outline_y) {
            if (gui_drag_outline_visible) {
                gui_draw_drag_outline(
                    gui_drag_outline_x,
                    gui_drag_outline_y,
                    active_window.w,
                    active_window.h
                );
            }

            gui_drag_outline_x = new_x;
            gui_drag_outline_y = new_y;

            gui_draw_drag_outline(
                gui_drag_outline_x,
                gui_drag_outline_y,
                active_window.w,
                active_window.h
            );

            gui_drag_outline_visible = 1;
        }
    }
    if (!left && gui_mouse_left_prev) {
        gui_mouse_left_released(x, y);
    }

    (void)right;
}
*/

static void gui_desktop_open_selected_app(void) {
    int old_visible = active_window.visible;
    int old_x = active_window.x;
    int old_y = active_window.y;
    int old_w = active_window.w;
    int old_h = active_window.h;

    /*
       Si había una ventana abierta, borramos solo esa zona.
    */
    if (old_visible) {
        active_window.visible = 0;
        gui_restore_desktop_region(old_x - 2, old_y - 2, old_w + 4, old_h + 4);
    }

    active_window.visible = 1;

    if (desktop_selected_icon == 0) {
        active_window.x = 260;
        active_window.y = 120;
        active_window.w = 1100;
        active_window.h = 720;
        active_window.title = "Files";
        active_window.app = DESKTOP_APP_FILES;

        gui_explorer_dir = fs_get_root();
        gui_explorer_selected = 0;

        /*
           Deja esta línea solo si tu código tiene gui_explorer_scroll.
           Si no existe, bórrala.
        */
        gui_explorer_scroll = 0;
    }

    else if (desktop_selected_icon == 1) {
        active_window.x = 320;
        active_window.y = 180;
        active_window.w = 1200;
        active_window.h = 650;
        active_window.title = "Terminal";
        active_window.app = DESKTOP_APP_TERMINAL;

        gui_term_clear_buffer();
        gui_term_clear_input();

        gui_term_add_line("Bionic graphical terminal");
        gui_term_add_line("Type help for commands");
    }

    else if (desktop_selected_icon == 2) {
        active_window.x = 420;
        active_window.y = 220;
        active_window.w = 760;
        active_window.h = 420;
        active_window.title = "About Bionic";
        active_window.app = DESKTOP_APP_INFO;
    }

    /*
       Dibujamos solo la ventana, no todo el escritorio.
    */
    gui_draw_desktop_window(&active_window);
}