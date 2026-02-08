#include "buttons.h"

#include "hardware/pio.h"
#include "pins.h"

void button_init(uint button) {
	gpio_init(button);
	gpio_set_dir(button, GPIO_IN);
	gpio_pull_up(button);
}

/**
 * Initialize hardware for all button inputs.
 *
 * Configures PIN_BTN_UP, PIN_BTN_DOWN, and PIN_BTN_OK for use (e.g., sets up
 * direction and pull state via pin_init).
 */
void buttons_init() {
	button_init(PIN_BTN_UP);
	button_init(PIN_BTN_DOWN);
	button_init(PIN_BTN_OK);
}

/**
 * Determine whether the specified button is currently pressed.
 *
 * @param button GPIO pin number associated with the button.
 * @returns `true` if the button is pressed (GPIO reads low), `false` otherwise.
 */
bool button_pressed(uint button) {
	return gpio_get(button) == 0;
}