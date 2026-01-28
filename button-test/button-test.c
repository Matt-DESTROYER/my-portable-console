#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/interp.h"

// Screen pins
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19
#define PIN_DC   20
#define PIN_RST  21

#define SPI_PORT spi0

// Button pins
#define PIN_BTN_UP   13
#define PIN_BTN_DOWN 14
#define PIN_BTN_OK   15

// RGB565 formatted colours
#define BLACK    0x0000
#define DARKGREY 0x2104
#define WHITE    0xFFFF
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0

// send command to lcd screen
void lcd_cmd(uint8_t cmd) {
    gpio_put(PIN_CS, 0); gpio_put(PIN_DC, 0);
    spi_write_blocking(SPI_PORT, &cmd, 1);
    gpio_put(PIN_CS, 1);
}

// send data to lcd screen
void lcd_data(uint8_t data) {
    gpio_put(PIN_CS, 0); gpio_put(PIN_DC, 1);
    spi_write_blocking(SPI_PORT, &data, 1);
    gpio_put(PIN_CS, 1);
}

// set window area on lcd screen
void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // set column address
    lcd_cmd(0x2A);
    lcd_data(x0 >> 8); lcd_data(x0 & 0xFF);
    lcd_data(x1 >> 8); lcd_data(x1 & 0xFF);

    // set row address
    lcd_cmd(0x2B);
    lcd_data(y0 >> 8); lcd_data(y0 & 0xFF);
    lcd_data(y1 >> 8); lcd_data(y1 & 0xFF);

    // memory write command
    lcd_cmd(0x2C);
}

// draw rectangle on lcd screen
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t colour) {
    lcd_set_window(x, y, x + w - 1, y + h - 1);

    uint8_t hi = colour >> 8;
    uint8_t lo = colour & 0xFF;

    uint8_t buffer[2] = {hi, lo};

    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 1);

    for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
        spi_write_blocking(SPI_PORT, buffer, 2);
    }
    gpio_put(PIN_CS, 1);
}

void lcd_init() {
    // reset the chip
    gpio_put(PIN_RST, 1); sleep_ms(5);
    gpio_put(PIN_RST, 0); sleep_ms(20);
    gpio_put(PIN_RST, 1); sleep_ms(150);

    lcd_cmd(0x01); sleep_ms(150); // software Reset

    // power & voltage settings
    lcd_cmd(0xCB); lcd_data(0x39); lcd_data(0x2C); lcd_data(0x00); lcd_data(0x34); lcd_data(0x02);
    lcd_cmd(0xCF); lcd_data(0x00); lcd_data(0xC1); lcd_data(0x30);
    lcd_cmd(0xE8); lcd_data(0x85); lcd_data(0x00); lcd_data(0x78);
    lcd_cmd(0xEA); lcd_data(0x00); lcd_data(0x00);
    lcd_cmd(0xED); lcd_data(0x64); lcd_data(0x03); lcd_data(0x12); lcd_data(0x81);
    lcd_cmd(0xF7); lcd_data(0x20);
    lcd_cmd(0xC0); lcd_data(0x23);
    lcd_cmd(0xC1); lcd_data(0x10);
    lcd_cmd(0xC5); lcd_data(0x3e); lcd_data(0x28);
    lcd_cmd(0xC7); lcd_data(0x86);

    lcd_cmd(0x36); lcd_data(0x48); // rotation/orientation
    lcd_cmd(0x3A); lcd_data(0x55); // pixel format (16-bit)
    lcd_cmd(0xB1); lcd_data(0x00); lcd_data(0x18);
    lcd_cmd(0xB6); lcd_data(0x08); lcd_data(0x82); lcd_data(0x27);

    lcd_cmd(0x11); sleep_ms(120); // sleep out
    lcd_cmd(0x29); sleep_ms(20);  // display on
}

void draw_menu_item(int y, int index, bool selected) {
    uint16_t box_colour = selected ? WHITE : DARKGREY;
    uint16_t dot_colour = selected ? RED : DARKGREY;

    lcd_fill_rect(40, y, 160, 40, box_colour);

    if (selected) {
        lcd_fill_rect(20, y + 10, 10, 20, YELLOW);
    } else {
        lcd_fill_rect(20, y + 10, 10, 20, BLACK);
    }
}

void draw_menu() {
    // clear screen
    lcd_fill_rect(0, 0, 240, 320, BLACK);

    // fake OS header
    lcd_fill_rect(0, 0, 240, 30, BLUE);
}

// kernel main
int main() {
    // init
    stdio_init_all();
    sleep_ms(1000);
    spi_init(SPI_PORT, 20000000); // 20 MHz
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    // init GPIO pins
    gpio_init(PIN_CS); gpio_set_dir(PIN_CS, GPIO_OUT); gpio_put(PIN_CS, 1);
    gpio_init(PIN_DC); gpio_set_dir(PIN_DC, GPIO_OUT); gpio_put(PIN_DC, 1);
    gpio_init(PIN_RST); gpio_set_dir(PIN_RST, GPIO_OUT); gpio_put(PIN_RST, 1);

    // initialinitise buttons
    gpio_init(PIN_BTN_UP); gpio_set_dir(PIN_BTN_UP, GPIO_IN); gpio_pull_up(PIN_BTN_UP);
    gpio_init(PIN_BTN_DOWN); gpio_set_dir(PIN_BTN_DOWN, GPIO_IN); gpio_pull_up(PIN_BTN_DOWN);
    gpio_init(PIN_BTN_OK); gpio_set_dir(PIN_BTN_OK, GPIO_IN); gpio_pull_up(PIN_BTN_OK);

    // init screen
    lcd_init();
    draw_menu();

    int selected_app = 0;
    int total_apps = 3;
    bool update_screen = true;

    // infinite loop
    while (1) {
        if (gpio_get(PIN_BTN_UP) == 0) {
            selected_app--;
            if (selected_app < 0) {
                selected_app = total_apps - 1;
            }
            update_screen = true;
            sleep_ms(150);
        }

        if (gpio_get(PIN_BTN_DOWN) == 0) {
            selected_app++;
            if (selected_app >= total_apps) {
                selected_app = 0;
            }
            update_screen = true;
            sleep_ms(150);
        }

        if (gpio_get(PIN_BTN_OK) == 0) {
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
