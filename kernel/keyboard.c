#include "../include/keyboard.h"
#include "../include/irq.h"
#include "../include/shell.h"
#include "../include/vga.h"
#include "../include/input.h"
#include "../include/gui.h"

#define KEYBOARD_DATA_PORT 0x60
#define SCANCODE_LEFT_SHIFT_PRESS    0x2A
#define SCANCODE_RIGHT_SHIFT_PRESS   0x36
#define SCANCODE_LEFT_SHIFT_RELEASE  0xAA
#define SCANCODE_RIGHT_SHIFT_RELEASE 0xB6

static int shift_pressed = 0;

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static const char scancode_map[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

static const char scancode_shift_map[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

static void keyboard_handler(void) {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    if (scancode == SCANCODE_LEFT_SHIFT_PRESS || scancode == SCANCODE_RIGHT_SHIFT_PRESS) {
        shift_pressed = 1;
        return;
    }

    if (scancode == SCANCODE_LEFT_SHIFT_RELEASE || scancode == SCANCODE_RIGHT_SHIFT_RELEASE) {
        shift_pressed = 0;
        return;
    }

    if (scancode & 0x80) {
        return;
    }

    if (scancode < sizeof(scancode_map)) {
        char c;

        if (shift_pressed) {
            c = scancode_shift_map[scancode];
        } else {
            c = scancode_map[scancode];
        }

    if (c) {
        if (input_get_mode() == INPUT_MODE_GUI) {
        gui_handle_key(c);
    } else {
        shell_handle_key(c);
    }
}
    }
}

void keyboard_init(void) {
    irq_register(1, keyboard_handler);
}