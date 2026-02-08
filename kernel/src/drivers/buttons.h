#ifndef KERNEL_BUTTONS_H
#define KERNEL_BUTTONS_H

#include <stdbool.h>
#include "pico/stdlib.h"

void buttons_init();
bool button_pressed(uint button);

#endif
