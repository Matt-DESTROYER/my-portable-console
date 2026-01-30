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

// SD pins
#define PIN_MISO 16
#define PIN_SDCS 22

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

bool test_sd_card() {
    uint8_t cmd0[] = { 0x40, 0x00, 0x00, 0x00, 0x00, 0x95 }; // GO_IDLE_STATE
    uint8_t response = 0xFF;

    // deselect everything
    gpio_put(PIN_CS, 1);
    gpio_put(PIN_SDCS, 1);

    // send 80 dummy clocks (10 bytes of 0xFF) to tell the SD card to wake up
    uint16_t dummy[] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
    spi_write_blocking(SPI_PORT, (uint8_t*)dummy, 10);

    // select SD card
    gpio_put(PIN_SDCS, 0);

    // semd CMD0 (reset)
    spi_write_blocking(SPI_PORT, cmd0, 6);

    // wait for response (expect 0x01)
    for (int i = 0; i < 10; i++) {
        spi_read_blocking(SPI_PORT, 0xFF, &response, 1);
        if (response != 0xFF) break;
    }

    gpio_put(PIN_SDCS, 1);

    printf("SD Card Response to CMD0: 0x%02X\n", response);

    // 0x01 == in idle state
    return (response == 0x01);
}

uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc) {
    // create packet
    uint8_t packet[6];
    packet[0] = 0x40 | cmd;
    packet[1] = (arg >> 24) & 0xFF;
    packet[2] = (arg >> 16) & 0xFF;
    packet[3] = (arg >> 8) & 0xFF;
    packet[4] = arg & 0xFF;
    packet[5] = crc;

    gpio_put(PIN_SDCS, 0);

    // wait for card to be ready
    uint8_t busy = 0;
    for (int i = 0; i < 100; i++) {
        spi_read_blocking(SPI_PORT, 0xFF, &busy, 1);
        if (busy == 0xFF) break;
    }

    // send command
    spi_write_blocking(SPI_PORT, packet, 6);

    // wait for response (starts with a 0, therefore < 0x80)
    uint8_t response = 0xFF;
    for (int i = 0; i < 100; i++) {
        spi_read_blocking(SPI_PORT, 0xFF, &response, 1);
        if ((response & 0x80) == 0) break;
    }

    return response;
}

bool sd_init() {
    uint8_t response = 0xFF;

    // deselect everything
    gpio_put(PIN_CS, 1);
    gpio_put(PIN_SDCS, 1);

    // send 80 dummy clocks (10 bytes of 0xFF) to tell the SD card to wake up
    uint16_t dummy[] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
    spi_write_blocking(SPI_PORT, (uint8_t*)dummy, 10);

    response = sd_send_cmd(0, 0, 0x95);
    gpio_put(PIN_SDCS, 1);
    if (response != 0x01) {
        printf("CMD0 failed, response: 0x%02X\n", response);
        return false;
    }

    // CMD8 check voltage
    printf("Sending CMD8\n");
    response = sd_send_cmd(8, 0x1AA, 0x87); // arg: 3.3V pattern
    gpio_put(PIN_SDCS, 1);

    // ACMD41 loop (wake up)
    // (send CMD55 + CMD41 until response is 0x00)
    printf("Sending ACMD41 loop\n");
    for (int i = 0; i < 1000; i++) {
        sd_send_cmd(55, 0, 0x65);
        gpio_put(PIN_SDCS, 1);

        // CMD41 (wake up, high capacity)
        response = sd_send_cmd(41, 0x40000000, 0x77);
        gpio_put(PIN_SDCS, 1);

        if (response == 0x00) {
            printf("SUCCESS: Card woke up!\n");
            return true;
        }
        sleep_ms(10);
    }

    printf("FAIL: ACMD41 timeout\n");
    return false;
}

// read a 512 byte sector
bool sd_read_sector(uint32_t sector, uint8_t* buffer) {
    // CMD17 == read single block
    // SDHC uses block addressing (0, 1, 2)
    // SDSC uses byte addressing (0, 512, 1024)
    // assume SDHC on modern cards
    uint8_t response = sd_send_cmd(17, sector, 0x00);

    if (response != 0x00) {
        printf("CMD17 failed, response: 0x%02X\n", response);
        gpio_put(PIN_SDCS, 1);
        return false;
    }

    // wait for start block token (0xFE)
    uint8_t token = 0xFF;
    for (int i = 0; i < 10000; i++) {
        spi_read_blocking(SPI_PORT, 0xFF, &token, 1);
        if (token == 0xFE) break;
        sleep_us(10);
    }

    if (token != 0xFE) {
        printf("Read timeout, token: 0x%02X\n", token);
        gpio_put(PIN_SDCS, 1);
        return false;
    }

    // read 512 bytes of data
    spi_read_blocking(SPI_PORT, 0xFF, buffer, 512);

    // read 2 bytes CRC (checksum)
    uint8_t crc[2];
    spi_read_blocking(SPI_PORT, 0xFF, crc, 2);

    // deselect
    gpio_put(PIN_SDCS, 1);

    return true;
}

// kernel main
int main() {
    // init
    stdio_init_all();
    sleep_ms(2000);

    spi_init(SPI_PORT, 1000000); // 1 MHz (for safety)
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

    // init GPIO pins
    gpio_init(PIN_CS); gpio_set_dir(PIN_CS, GPIO_OUT); gpio_put(PIN_CS, 1);
    gpio_init(PIN_SDCS); gpio_set_dir(PIN_SDCS, GPIO_OUT); gpio_put(PIN_SDCS, 1);
    gpio_init(PIN_DC); gpio_set_dir(PIN_DC, GPIO_OUT); gpio_put(PIN_DC, 1);
    gpio_init(PIN_RST); gpio_set_dir(PIN_RST, GPIO_OUT); gpio_put(PIN_RST, 1);

    // wake up sequence (dumb dummy clocks)
    gpio_put(PIN_CS, 1); gpio_put(PIN_SDCS, 1);
    uint8_t dummy = 0xFF;
    for (int i = 0; i < 10; i++) {
        spi_write_blocking(SPI_PORT, &dummy, 1);
    }

    // reset (CMD0)
    uint8_t r = sd_send_cmd(0, 0, 0x95);
    printf("CMD0 response: 0x02X\n", r);

    // technically we need to init with CMD8 and ACMD41

    if (sd_init()) {
        uint8_t buffer[512];
        if (sd_read_sector(0, buffer)) {
            printf("Sector 0 dump:\n");

            // print the first 64 chars as ASCII
            for (int i = 0; i < 512; i++) {
                if (i % 16 == 0) {
                    printf("\n%04X: ", i);
                }
                if (buffer[i] >= 32 && buffer[i] < 127) {
                    printf("%c", buffer[i]);
                } else {
                    printf(".");
                }
            }
            printf("\n\n SUCCESS: found filesystem signature!\n");
        } else {
            printf("FAIL: might need the full 'ACMD41' init sequence");
        }
    }

    // NOTE you will have to manually hold BOOTSEL
    // and transfer the driver if you don't have an
    // infinite loop

    /*
    while (1) {
        if (test_sd_card()) {
            printf("SUCCESS: SD card detected!\n");
        } else {
            printf("FAIL: SD card is silent...\n");
        }
        sleep_ms(1000);
    }
    */
}
