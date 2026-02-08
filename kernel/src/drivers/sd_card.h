#ifndef KERNEL_SD_CARD_H
#define KERNEL_SD_CARD_H

#include <stdint.h>
#include <stdbool.h>

bool test_sd_card();
uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc);
bool sd_init();
bool sd_read_sector(uint32_t sector, uint8_t* buffer);

#endif
