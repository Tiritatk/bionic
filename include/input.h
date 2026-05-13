#ifndef INPUT_H
#define INPUT_H

typedef enum {
    INPUT_MODE_SHELL,
    INPUT_MODE_GUI
} input_mode_t;

void input_set_mode(input_mode_t mode);
input_mode_t input_get_mode(void);

#endif