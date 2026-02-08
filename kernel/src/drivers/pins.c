#include "pins.h"

/**
 * Initialize a GPIO pin: enable it, configure it as an output, and drive it high.
 *
 * @param gpio GPIO pin number or identifier to initialize.
 */
void pin_init(uint gpio) {
	gpio_init(gpio);
	gpio_set_dir(gpio, GPIO_OUT);
	gpio_put(gpio, 1);
}