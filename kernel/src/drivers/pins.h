#ifndef KERNEL_PINS_H
#define KERNEL_PINS_H

#include "pico/stdlib.h"
#include "hardware/spi.h"

// Screen pins
#define PIN_CS       17
#define PIN_SCK      18
#define PIN_MOSI     19
#define PIN_DC       20
#define PIN_RST      21

// SD pins
#define PIN_MISO     16
#define PIN_SDCS     22

#define SPI_PORT     spi0

// Button pins
#define PIN_BTN_UP   13
#define PIN_BTN_DOWN 14
#define PIN_BTN_OK   15

void pin_init(uint gpio);

#endif
