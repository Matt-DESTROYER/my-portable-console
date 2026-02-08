#include "sd_card.h"

//#include <stdio.h>

#include "pins.h"

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

	// send CMD0 (reset)
	spi_write_blocking(SPI_PORT, cmd0, 6);

	// wait for response (expect 0x01)
	for (int i = 0; i < 10; i++) {
		spi_read_blocking(SPI_PORT, 0xFF, &response, 1);
		if (response != 0xFF) break;
	}

	gpio_put(PIN_SDCS, 1);

	//printf("SD Card Response to CMD0: 0x%02X\n", response);

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
		//printf("CMD0 failed, response: 0x%02X\n", response);
		return false;
	}

	// CMD8 check voltage
	//printf("Sending CMD8\n");
	response = sd_send_cmd(8, 0x1AA, 0x87); // arg: 3.3V pattern
	uint8_t r7[4];
	spi_read_blocking(SPI_PORT, 0xFF, r7, 4);
	gpio_put(PIN_SDCS, 1);

	// ACMD41 loop (wake up)
	// (send CMD55 + CMD41 until response is 0x00)
	//printf("Sending ACMD41 loop\n");
	for (int i = 0; i < 1000; i++) {
		sd_send_cmd(55, 0, 0x65);
		gpio_put(PIN_SDCS, 1);

		// CMD41 (wake up, high capacity)
		response = sd_send_cmd(41, 0x40000000, 0x77);
		gpio_put(PIN_SDCS, 1);

		if (response == 0x00) {
			//printf("SUCCESS: Card woke up!\n");
			return true;
		}
		sleep_ms(10);
	}

	//printf("FAIL: ACMD41 timeout\n");
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
		//printf("CMD17 failed, response: 0x%02X\n", response);
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
		//printf("Read timeout, token: 0x%02X\n", token);
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
