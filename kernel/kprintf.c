#include "../include/kprintf.h"
#include "../include/vga.h"

/* Implementación mínima de va_list sin stdarg.h */
typedef __builtin_va_list va_list;
#define va_start(v, l)  __builtin_va_start(v, l)
#define va_arg(v, l)    __builtin_va_arg(v, l)
#define va_end(v)       __builtin_va_end(v)

/* Convierte un número entero sin signo a string en la base indicada */
static void print_uint(uint32_t n, uint32_t base, int pad, char padchar) {
    static const char digits[] = "0123456789abcdef";
    char buf[32];
    int  i = 0;

    if (n == 0) {
        buf[i++] = '0';
    } else {
        while (n > 0) {
            buf[i++] = digits[n % base];
            n /= base;
        }
    }

    /* Relleno por la izquierda */
    while (i < pad)
        buf[i++] = padchar;

    /* Imprime al revés (buf está invertido) */
    while (i-- > 0)
        vga_putchar(buf[i]);
}

static void print_int(int32_t n, int pad, char padchar) {
    if (n < 0) {
        vga_putchar('-');
        print_uint((uint32_t)(-n), 10, pad, padchar);
    } else {
        print_uint((uint32_t)n, 10, pad, padchar);
    }
}

static void kprintf_internal(const char *fmt, va_list args) {
    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            vga_putchar(*fmt);
            continue;
        }

        fmt++;  /* salta el '%' */

        /* Padding: detecta '0' o número antes del especificador */
        char padchar = ' ';
        int  pad     = 0;

        if (*fmt == '0') {
            padchar = '0';
            fmt++;
        }
        while (*fmt >= '0' && *fmt <= '9') {
            pad = pad * 10 + (*fmt - '0');
            fmt++;
        }

        switch (*fmt) {
            case 'c':
                vga_putchar((char)va_arg(args, int));
                break;

            case 's': {
                const char *s = va_arg(args, const char *);
                if (!s) s = "(null)";
                vga_puts(s);
                break;
            }

            case 'd':
                print_int(va_arg(args, int32_t), pad, padchar);
                break;

            case 'u':
                print_uint(va_arg(args, uint32_t), 10, pad, padchar);
                break;

            case 'x':
                print_uint(va_arg(args, uint32_t), 16, pad, padchar);
                break;

            case 'X': {
                /* Igual que %x pero con prefijo 0x */
                uint32_t v = va_arg(args, uint32_t);
                vga_puts("0x");
                print_uint(v, 16, pad, padchar);
                break;
            }

            case 'b':
                /* Binario, útil para flags y registros */
                print_uint(va_arg(args, uint32_t), 2, pad, padchar);
                break;

            case '%':
                vga_putchar('%');
                break;

            default:
                vga_putchar('?');
                break;
        }
    }
}

void kprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    kprintf_internal(fmt, args);
    va_end(args);
}

void kprintf_color(vga_color_t fg, vga_color_t bg, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vga_set_color(fg, bg);
    kprintf_internal(fmt, args);
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);  /* restaura color por defecto */
    va_end(args);
}