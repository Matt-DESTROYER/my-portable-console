#ifndef KERNEL_MEMORY_H
#define KERNEL_MEMORY_H

#include <stdint.h>

extern uint8_t __end__;
extern uint8_t __StackLimit;

uint8_t* heap_start();
uint8_t* heap_end();
size_t total_free_bytes();

#endif
