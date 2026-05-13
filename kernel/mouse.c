#include "../include/mouse.h"
#include "../include/io.h"
#include "../include/gui.h"
#include "../include/irq.h"

#define PS2_DATA_PORT    0x60
#define PS2_STATUS_PORT  0x64
#define PS2_COMMAND_PORT 0x64

static int32_t mouse_x = 100;
static int32_t mouse_y = 100;

static uint8_t mouse_packet[3];
static uint8_t mouse_cycle = 0;

static uint8_t left_button = 0;
static uint8_t right_button = 0;

static void mouse_wait_input(void) {
    uint32_t timeout = 100000;

    while (timeout--) {
        if ((inb(PS2_STATUS_PORT) & 0x02) == 0) {
            return;
        }
    }
}

static void mouse_wait_output(void) {
    uint32_t timeout = 100000;

    while (timeout--) {
        if (inb(PS2_STATUS_PORT) & 0x01) {
            return;
        }
    }
}

static void mouse_write(uint8_t value) {
    mouse_wait_input();
    outb(PS2_COMMAND_PORT, 0xD4);

    mouse_wait_input();
    outb(PS2_DATA_PORT, value);
}

static uint8_t mouse_read(void) {
    mouse_wait_output();
    return inb(PS2_DATA_PORT);
}

void mouse_init(void) {
    mouse_cycle = 0;
    left_button = 0;
    right_button = 0;

    mouse_x = 100;
    mouse_y = 100;

    /*
       Activar dispositivo auxiliar PS/2.
    */
    mouse_wait_input();
    outb(PS2_COMMAND_PORT, 0xA8);

    /*
       Activar IRQ12 en el controlador PS/2.
    */
    mouse_wait_input();
    outb(PS2_COMMAND_PORT, 0x20);

    mouse_wait_output();
    uint8_t status = inb(PS2_DATA_PORT);

    status |= 0x02;

    mouse_wait_input();
    outb(PS2_COMMAND_PORT, 0x60);

    mouse_wait_input();
    outb(PS2_DATA_PORT, status);

    /*
       Configuración básica del mouse.
    */
    mouse_write(0xF6); // set defaults
    mouse_read();

    mouse_write(0xF4); // enable data reporting
    mouse_read();
    
    irq_register(12, mouse_handle_interrupt);
}

void mouse_handle_interrupt(void) {
    uint8_t data = inb(PS2_DATA_PORT);

    if (mouse_cycle == 0) {
        /*
           El primer byte de un paquete PS/2 debe tener el bit 3 activo.
           Si no, ignoramos para evitar desincronización.
        */
        if (!(data & 0x08)) {
            return;
        }

        mouse_packet[0] = data;
        mouse_cycle = 1;
        return;
    }

    if (mouse_cycle == 1) {
        mouse_packet[1] = data;
        mouse_cycle = 2;
        return;
    }

    mouse_packet[2] = data;
    mouse_cycle = 0;

    int32_t dx = (int8_t)mouse_packet[1];
    int32_t dy = (int8_t)mouse_packet[2];

left_button = (mouse_packet[0] & 0x01) ? 1 : 0;
right_button = (mouse_packet[0] & 0x02) ? 1 : 0;

    mouse_x += dx;
    mouse_y -= dy;

    if (mouse_x < 0) {
        mouse_x = 0;
    }

    if (mouse_y < 0) {
        mouse_y = 0;
    }

    if (mouse_x >= (int32_t)gui_get_width()) {
        mouse_x = gui_get_width() - 1;
    }

    if (mouse_y >= (int32_t)gui_get_height()) {
        mouse_y = gui_get_height() - 1;
    }

    gui_mouse_moved(mouse_x, mouse_y, left_button, right_button);
}

int32_t mouse_get_x(void) {
    return mouse_x;
}

int32_t mouse_get_y(void) {
    return mouse_y;
}

uint8_t mouse_left_down(void) {
    return left_button;
}

uint8_t mouse_right_down(void) {
    return right_button;
}