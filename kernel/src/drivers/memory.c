#include "memory.h"

/**
 * Get the start address of the heap region.
 *
 * @returns Pointer to the start of the heap (address of the linker-provided `__end__` symbol).
 */
uint8_t* heap_start() {
	return &__end__;
}
/**
 * Provide the end address of the heap region.
 * @return Pointer to the heap end (address of the linker symbol `__StackLimit`) as `uint8_t *`.
 */
uint8_t* heap_end() {
	return &__StackLimit;
}
/**
 * Compute the number of free bytes remaining in the heap after reserving a 1KB safety margin.
 *
 * @returns Number of free bytes available for allocation after reserving 1024 bytes (returned as a `uint8_t`).
 */
#define KERNEL_MEMORY_SAFETY 1024 // 1 KB safety margin
size_t total_free_bytes() {
	size_t available =  heap_end() - heap_start();
	if (available < KERNEL_MEMORY_SAFETY) {
		return 0;
	}
	return available - KERNEL_MEMORY_SAFETY;
}