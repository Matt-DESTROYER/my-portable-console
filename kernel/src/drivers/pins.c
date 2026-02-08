#include "pins.h"

void pin_init(uint gpio) {
	gpio_init(gpio);
	gpio_set_dir(gpio, GPIO_OUT);
	gpio_put(gpio, 1);
}
