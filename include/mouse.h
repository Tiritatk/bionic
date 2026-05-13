#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

void mouse_init(void);
void mouse_handle_interrupt(void);

int32_t mouse_get_x(void);
int32_t mouse_get_y(void);

uint8_t mouse_left_down(void);
uint8_t mouse_right_down(void);

#endif