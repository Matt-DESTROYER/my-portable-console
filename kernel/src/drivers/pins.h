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

#define DEFAULT_MHZ  62500000 //  62.5 MHz
#define JUMPER_MHZ   31250000 //  31.25 MHz
#define SD_INIT_MHZ  400000   // 400 kHz
#define SD_MHZ       20000000 //  20.0 MHz

#ifdef JUMPER_WIRES
#undef DEFAULT_MHZ
#define DEFAULT_MHZ JUMPER_MHZ
#endif

void pin_init(uint gpio);

#endif
