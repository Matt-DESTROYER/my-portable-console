#include "buttons.h"

#include "hardware/pio.h"
#include "pins.h"

/**
 * Initialize hardware for all button inputs.
 *
 * Configures PIN_BTN_UP, PIN_BTN_DOWN, and PIN_BTN_OK for use (e.g., sets up
 * direction and pull state via pin_init).
 */
void buttons_init() {
	pin_init(PIN_BTN_UP);
	pin_init(PIN_BTN_DOWN);
	pin_init(PIN_BTN_OK);
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