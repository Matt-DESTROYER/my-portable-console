#ifndef KERNEL_SD_CARD_H
#define KERNEL_SD_CARD_H

#include <stdint.h>

#include "pico/stdlib.h"

// 4.7.1 Command Types
typedef enum CommandType {
	BC,  // broadcast (no response)
	BCR, // broadcast (with) response
	AC,  // addressed (point-to-point) no data transfer on DAT
	ADTC // addressed (point-to-point) data transfer on DAT
} CommandType_t;

// 4.7.2 Command Format
typedef struct Command {
	uint8_t start         :  1;
	uint8_t transmission  :  1;
	uint8_t command_index :  6;
	uint32_t argument     : 32;
	uint8_t crc7          :  7;
	uint8_t end           :  1;
} Command_t;

// 4.9 Responses
typedef struct Response {
	uint8_t start         :  1;
	uint8_t transmission  :  1;
	uint8_t command_index :  6;
	uint32_t status       : 32;
	uint8_t crc7          :  7;
	uint8_t end           :  1;
} Response_t;
typedef struct Response Response1_t;
typedef Response1_t R1_t;

typedef struct Response2 {
	uint8_t start        :  1;
	uint8_t transmission :  1;
	uint8_t reserved     :  6;
	uint64_t register1   : 64;
	uint64_t register2   : 63;
	uint64_t end         :  1;
} Response2_t;
typedef Response2_t R2_t;

typedef struct Response3 {
	uint8_t start         :  1;
	uint8_t transmission  :  1;
	uint8_t reserved1     :  6;
	uint32_t ocr_register : 32;
	uint8_t reserved2     :  7;
	uint8_t end           :  1;
} Response3_t;
typedef Response3_t R3_t;

typedef struct Response6 {
	uint8_t start         :  1;
	uint8_t transmission  :  1;
	uint8_t command_index :  6;
	uint32_t rca          : 16;
	uint32_t card_status  : 16;
	uint8_t crc7          :  7;
	uint8_t end           :  1;
} Response6_t;
typedef Response6_t R6_t;

typedef struct Response7 {
	uint8_t start : 1;
	uint8_t transmission : 1;
	uint8_t command_index : 6;
	uint32_t reserved1 : 18;
	uint32_t pcie_1_2V : 1;
	uint32_t pcie_response : 1;
	uint32_t voltage_accepted : 4;
	uint16_t pattern_check : 8;
	uint16_t crc7 : 7;
	uint16_t end;
} Response7_t;
typedef Response7_t R7_t;

#define VOLTAGE_ACCEPTED_NOT_DEFINED       0b0000
#define VOLTAGE_ACCEPTED_2_7V_3_6V         0b0001
#define VOLTAGE_ACCEPTED_LOW_VOLTAGE_RANGE 0b0010
#define VOLTAGE_ACCEPTED_RESERVED1         0b0100
#define VOLTAGE_ACCEPTED_RESERVED2         0b1000

#endif
