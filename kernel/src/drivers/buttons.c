#include "buttons.h"

#include "hardware/pio.h"
#include "pins.h"

void buttons_init() {
	pin_init(PIN_BTN_UP);
	pin_init(PIN_BTN_DOWN);
	pin_init(PIN_BTN_OK);
}

bool button_pressed(uint button) {
	return gpio_get(button) == 0;
}
