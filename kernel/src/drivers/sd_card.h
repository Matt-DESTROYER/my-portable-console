#ifndef KERNEL_SD_CARD_H
#define KERNEL_SD_CARD_H

#include "pico/stdlib.h"

typedef enum CommandType {
	BC,  // broadcast (no response)
	BCR, // broadcast (with) response
	AC,  // addressed (point-to-point) no data transfer on DAT
	ADTC // addressed (point-to-point) data transfer on DAT
} CommandType_t;

typedef struct Command {
	uint8_t start         :  1;
	uint8_t transmission  :  1;
	uint8_t command_index :  6;
	uint32_t argument     : 32;
	uint8_t CRC7          :  7;
	uint8_t end           :  1;
} Command_t;

#endif
