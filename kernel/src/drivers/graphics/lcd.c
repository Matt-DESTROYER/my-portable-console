#include "lcd.h"

#include "../pins.h"

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
