#include "../include/input.h"

static input_mode_t current_input_mode = INPUT_MODE_SHELL;

void input_set_mode(input_mode_t mode) {
    current_input_mode = mode;
}

input_mode_t input_get_mode(void) {
    return current_input_mode;
}