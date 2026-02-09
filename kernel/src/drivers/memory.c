#include "memory.h"

uint8_t* heap_start() {
	return &__end__;
}
uint8_t* heap_end() {
	return &__StackLimit;
}
uint8_t total_free_bytes() {
	return (heap_end() - heap_start()) - 1024; // 1KB safety margin
}
