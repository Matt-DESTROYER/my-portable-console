#include "buttons.h"

#include "hardware/pio.h"
#include "pins.h"

void button_init(uint button) {
	gpio_init(button);
	gpio_set_dir(button, GPIO_IN);
	gpio_pull_up(button);
}

void buttons_init() {
	button_init(PIN_BTN_UP);
	button_init(PIN_BTN_DOWN);
	button_init(PIN_BTN_OK);
}

bool button_pressed(uint button) {
	return gpio_get(button) == 0;
}
