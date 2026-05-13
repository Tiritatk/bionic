#ifndef KPRINTF_H
#define KPRINTF_H

#include "vga.h"

void kprintf(const char *fmt, ...);
void kprintf_color(vga_color_t fg, vga_color_t bg, const char *fmt, ...);

#endif