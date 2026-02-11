#include "sd_card.h"

uint8_t crc7(uint8_t* buffer, size_t size) {
	// From the 4.5 of the datasheet
	// G(x) = 1*x^7 + 0*x^6 + 0*x^5 + 0*x^4 + 1*x^3 + 0*x^2 + 0*x^1 + 1*x^0
	//      =   x^7 + 0     + 0     + 0     +   x^3 + 0     + 0     + 1
	//      = x^7 + x^3 + 1
	// M(x) = (first bit) * x^n + (second bit) * x^(n-1) + ... + (last bit) * x^0
	// CRC[6..0] = Remainder [(M(x) * x^7) / G(x)]

	// Referenced https://en.wikipedia.org/wiki/Cyclic_redundancy_check#Computation

	uint8_t G = CRC7_POLYNOMIAL;

	uint8_t crc = 0b00000000;
	
	for (size_t i = 0; i < size; i++) {
		for (uint8_t j = 0; j < 8; j++) {
			crc <<= 1;
			crc |= (buffer[i] >> (7 - j)) & 0x01;
			if ((crc & 0x80) > 0) crc ^= CRC7_POLYNOMIAL;
		}
	}

	return crc;
}
