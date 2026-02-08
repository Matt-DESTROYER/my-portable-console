#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/interp.h"

#include "drivers/pins.h"
#include "drivers/graphics/lcd.h"
#include "drivers/graphics/os.h"
#include "drivers/sd_card.h"
#include "drivers/buttons.h"

/**
 * Initialize system peripherals and run the interactive LCD menu loop.
 *
 * Initializes stdio, SPI, GPIO pins, buttons, and the LCD, then enters an infinite
 * polling loop that handles UP/DOWN navigation with wrap-around, OK to temporarily
 * fill the display and refresh the menu, and redraws the visible menu items when
 * selection changes.
 *
 * @returns Exit status code; under normal operation this function does not return. 
 */
int main() {
	// initialise
	stdio_init_all();
	sleep_ms(2000);

	spi_init(SPI_PORT, DEFAULT_MHZ); // 62.5 MHz

    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

	pin_init(PIN_CS);
	pin_init(PIN_DC);
	pin_init(PIN_RST);
	pin_init(PIN_SDCS);

	buttons_init();

	lcd_init();
	draw_menu();

	int selected_app = 0;
    int total_apps = 3;
    bool update_screen = true;

    while (1) {
        if (button_pressed(PIN_BTN_UP)) {
            selected_app--;
            if (selected_app < 0) {
                selected_app = total_apps - 1;
            }
            update_screen = true;
            sleep_ms(150);
        }

        if (button_pressed(PIN_BTN_DOWN)) {
            selected_app++;
            if (selected_app >= total_apps) {
                selected_app = 0;
            }
            update_screen = true;
            sleep_ms(150);
        }

        if (button_pressed(PIN_BTN_OK)) {
            lcd_fill_rect(0, 0, 240, 320, GREEN);
            sleep_ms(200);
            draw_menu();
            update_screen = true;
            sleep_ms(200);
        }

        if (update_screen) {
            draw_menu_item(60,  0, (selected_app == 0));
            draw_menu_item(110, 1, (selected_app == 1));
            draw_menu_item(160, 2, (selected_app == 2));
            update_screen = false;
        }
    }
}