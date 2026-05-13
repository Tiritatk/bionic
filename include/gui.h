#ifndef GUI_H
#define GUI_H

#include <stdint.h>
#include "multiboot.h"

void gui_init_from_multiboot(multiboot_info_t* mb);
int gui_has_framebuffer(void);
void gui_demo(void);
void gui_handle_key(char c);

void gui_putpixel(uint32_t x, uint32_t y, uint8_t color);
void gui_clear(uint8_t color);
void gui_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t color);
void gui_rect_border(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t color);

void gui_draw_char(uint32_t x, uint32_t y, char c, uint8_t color);
void gui_draw_text(uint32_t x, uint32_t y, const char* text, uint8_t color);

void gui_label(uint32_t x, uint32_t y, const char* text, uint8_t color);
void gui_panel(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t bg, uint8_t border);
void gui_window(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const char* title);
void gui_button(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const char* text);
void gui_statusbar(const char* text);
uint32_t gui_get_width(void);
uint32_t gui_get_height(void);

void gui_mouse_moved(int32_t x, int32_t y, uint8_t left, uint8_t right);
// void gui_titlebar(uint32_t x, uint32_t y, uint32_t w, const char* title);

#endif