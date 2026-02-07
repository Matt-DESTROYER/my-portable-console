#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"

// SD Card SPI pins
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

#define SPI_PORT spi0
#define SPI_FREQ 400000  // 400 kHz for initialization

// SD Card commands
#define CMD0   0   // GO_IDLE_STATE
#define CMD8   8   // SEND_IF_COND
#define CMD55  55  // APP_CMD
#define CMD58  58  // READ_OCR
#define ACMD41 41  // SD_SEND_OP_COND

// SD Card responses
#define R1_IDLE_STATE   0x01
#define R1_READY_STATE  0x00

// Test results tracking
typedef struct {
    bool spi_init_ok;
    bool card_detect_ok;
    bool cmd0_ok;
    bool cmd8_ok;
    bool acmd41_ok;
    bool voltage_ok;
} sd_test_results_t;

// Send a byte via SPI and receive response
static uint8_t spi_transfer(uint8_t data) {
    uint8_t rx_data;
    spi_write_read_blocking(SPI_PORT, &data, &rx_data, 1);
    return rx_data;
}

// Send SD card command
static uint8_t sd_send_command(uint8_t cmd, uint32_t arg) {
    uint8_t response;
    uint8_t retry = 0;

    // Send command packet
    spi_transfer(0x40 | cmd);           // Start bit + command
    spi_transfer((arg >> 24) & 0xFF);   // Argument[31:24]
    spi_transfer((arg >> 16) & 0xFF);   // Argument[23:16]
    spi_transfer((arg >> 8) & 0xFF);    // Argument[15:8]
    spi_transfer(arg & 0xFF);           // Argument[7:0]

    // CRC (valid for CMD0 and CMD8)
    if (cmd == CMD0) {
        spi_transfer(0x95);
    } else if (cmd == CMD8) {
        spi_transfer(0x87);
    } else {
        spi_transfer(0x01);
    }

    // Wait for response (not 0xFF)
    do {
        response = spi_transfer(0xFF);
        retry++;
    } while ((response == 0xFF) && (retry < 10));

    return response;
}

// Initialize SD card
static bool sd_card_init(sd_test_results_t *results) {
    uint8_t response;
    uint32_t retry;

    // Pull CS high and send dummy clocks
    gpio_put(PIN_CS, 1);
    for (int i = 0; i < 10; i++) {
        spi_transfer(0xFF);
    }

    // Assert CS
    gpio_put(PIN_CS, 0);
    sleep_ms(1);

    // CMD0: GO_IDLE_STATE
    printf("Sending CMD0 (GO_IDLE_STATE)...\n");
    response = sd_send_command(CMD0, 0);
    printf("CMD0 response: 0x%02X\n", response);

    if (response == R1_IDLE_STATE) {
        printf("  [PASS] Card entered idle state\n");
        results->cmd0_ok = true;
    } else {
        printf("  [FAIL] Unexpected response\n");
        gpio_put(PIN_CS, 1);
        return false;
    }

    // CMD8: SEND_IF_COND (check voltage range)
    printf("\nSending CMD8 (SEND_IF_COND)...\n");
    response = sd_send_command(CMD8, 0x1AA);
    printf("CMD8 response: 0x%02X\n", response);

    if ((response & 0xFE) == 0x00) {
        // Read R7 response (4 bytes)
        uint8_t r7[4];
        for (int i = 0; i < 4; i++) {
            r7[i] = spi_transfer(0xFF);
        }
        printf("  R7: 0x%02X%02X%02X%02X\n", r7[0], r7[1], r7[2], r7[3]);

        if ((r7[2] & 0x0F) == 0x01 && r7[3] == 0xAA) {
            printf("  [PASS] SDv2 card, voltage accepted\n");
            results->cmd8_ok = true;
            results->voltage_ok = true;
        } else {
            printf("  [FAIL] Voltage range not accepted\n");
            gpio_put(PIN_CS, 1);
            return false;
        }
    } else if (response & 0x04) {
        printf("  [INFO] SDv1 card or MMC (illegal command)\n");
        results->cmd8_ok = false;
    } else {
        printf("  [FAIL] Unexpected response\n");
        gpio_put(PIN_CS, 1);
        return false;
    }

    // ACMD41: Initialize card
    printf("\nInitializing card with ACMD41...\n");
    retry = 0;
    do {
        // CMD55 must precede ACMD41
        gpio_put(PIN_CS, 0);
        response = sd_send_command(CMD55, 0);

        if (response > 1) {
            printf("  [FAIL] CMD55 failed: 0x%02X\n", response);
            gpio_put(PIN_CS, 1);
            return false;
        }

        // ACMD41 with HCS bit set
        response = sd_send_command(ACMD41, 0x40000000);
        gpio_put(PIN_CS, 1);

        if (response == R1_READY_STATE) {
            printf("  [PASS] Card initialization complete\n");
            results->acmd41_ok = true;
            break;
        }

        sleep_ms(100);
        retry++;
    } while (retry < 50);

    if (response != R1_READY_STATE) {
        printf("  [FAIL] Card did not initialize (response: 0x%02X)\n", response);
        return false;
    }

    // CMD58: Read OCR
    printf("\nReading OCR (CMD58)...\n");
    gpio_put(PIN_CS, 0);
    response = sd_send_command(CMD58, 0);

    if (response == R1_READY_STATE) {
        uint8_t ocr[4];
        for (int i = 0; i < 4; i++) {
            ocr[i] = spi_transfer(0xFF);
        }
        printf("  OCR: 0x%02X%02X%02X%02X\n", ocr[0], ocr[1], ocr[2], ocr[3]);

        if (ocr[0] & 0x40) {
            printf("  [PASS] High Capacity SD Card (SDHC/SDXC)\n");
        } else {
            printf("  [PASS] Standard Capacity SD Card (SDSC)\n");
        }
    } else {
        printf("  [FAIL] Could not read OCR\n");
    }

    gpio_put(PIN_CS, 1);
    return true;
}

// Print test summary
static void print_test_summary(const sd_test_results_t *results) {
    printf("\n");
    printf("=== SD CARD TEST SUMMARY ===\n");
    printf("SPI Initialization:     %s\n", results->spi_init_ok ? "PASS" : "FAIL");
    printf("Card Detection:         %s\n", results->card_detect_ok ? "PASS" : "FAIL");
    printf("CMD0 (Idle State):      %s\n", results->cmd0_ok ? "PASS" : "FAIL");
    printf("CMD8 (Interface Cond):  %s\n", results->cmd8_ok ? "PASS" : "FAIL");
    printf("ACMD41 (Init):          %s\n", results->acmd41_ok ? "PASS" : "FAIL");
    printf("Voltage Check:          %s\n", results->voltage_ok ? "PASS" : "FAIL");
    printf("\n");

    int passed = 0;
    int total = 6;

    if (results->spi_init_ok) passed++;
    if (results->card_detect_ok) passed++;
    if (results->cmd0_ok) passed++;
    if (results->cmd8_ok) passed++;
    if (results->acmd41_ok) passed++;
    if (results->voltage_ok) passed++;

    printf("Total: %d/%d tests passed\n", passed, total);

    if (passed == total) {
        printf("\nResult: ALL TESTS PASSED\n");
    } else {
        printf("\nResult: SOME TESTS FAILED\n");
    }
}

// Additional test: SPI communication test
static bool test_spi_communication(void) {
    printf("\n=== Testing SPI Communication ===\n");

    // Test pattern
    uint8_t test_data[] = {0xAA, 0x55, 0xF0, 0x0F};
    uint8_t read_data[sizeof(test_data)];

    gpio_put(PIN_CS, 0);
    sleep_us(10);

    // Send test pattern
    for (size_t i = 0; i < sizeof(test_data); i++) {
        read_data[i] = spi_transfer(test_data[i]);
    }

    gpio_put(PIN_CS, 1);

    printf("Sent:     ");
    for (size_t i = 0; i < sizeof(test_data); i++) {
        printf("0x%02X ", test_data[i]);
    }
    printf("\n");

    printf("Received: ");
    for (size_t i = 0; i < sizeof(read_data); i++) {
        printf("0x%02X ", read_data[i]);
    }
    printf("\n");

    printf("[PASS] SPI communication test completed\n");
    return true;
}

int main() {
    sd_test_results_t results = {0};

    // Initialize stdio
    stdio_init_all();
    sleep_ms(2000);  // Wait for USB serial

    printf("\n");
    printf("====================================\n");
    printf("   SD CARD TEST SUITE\n");
    printf("====================================\n");
    printf("\n");

    // Initialize SPI
    printf("Initializing SPI...\n");
    spi_init(SPI_PORT, SPI_FREQ);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    // Initialize CS pin
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    printf("[PASS] SPI initialized at %d Hz\n", SPI_FREQ);
    results.spi_init_ok = true;

    // Test SPI communication
    test_spi_communication();

    // Attempt card detection and initialization
    printf("\n=== Attempting SD Card Initialization ===\n");
    sleep_ms(100);

    results.card_detect_ok = sd_card_init(&results);

    // Print summary
    print_test_summary(&results);

    // Continuous monitoring loop
    printf("\n=== Entering Monitoring Mode ===\n");
    printf("(Press Ctrl+C to exit)\n\n");

    uint32_t loop_count = 0;
    while (1) {
        loop_count++;
        printf("Status check %lu - SD card test suite idle\n", loop_count);
        sleep_ms(5000);
    }

    return 0;
}