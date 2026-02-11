#ifndef KERNEL_SD_CARD_H
#define KERNEL_SD_CARD_H

#include "pico/stdlib.h"

typedef enum CommandType {
	BC,  // broadcast (no response)
	BCR, // broadcast (with) response
	AC,  // addressed (point-to-point) no data transfer on DAT
	ADTC // addressed (point-to-point) data transfer on DAT
} CommandType_t;

#endif
