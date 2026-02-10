#ifndef KERNEL_ALLOCATOR_H
#define KERNEL_ALLOCATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>

typedef struct MemoryHeader MemoryHeader_t;
/**
 * Memory block header structure.
 * 
 * The compiler automatically adds padding between in_use and next to ensure
 * proper pointer alignment (4-byte on 32-bit ARM). We explicitly request
 * 8-byte alignment for optimal performance on ARM Cortex-M33 (RP2350)
 * using C11 standard alignas.
 * 
 * Layout on 32-bit:
 *   size_t size;        [4 bytes, offset 0]
 *   bool in_use;        [1 byte,  offset 4]
 *   [padding]           [3 bytes, offset 5-7] (automatic)
 *   MemoryHeader_t* next; [4 bytes, offset 8]
 *   [padding to 16]     [4 bytes, offset 12-15] (for 8-byte alignment)
 * Total: 16 bytes
 */
struct MemoryHeader {
	size_t size;
	bool in_use;
	MemoryHeader_t* next;
} alignas(8);

void alloc_init(uint8_t* heap_start, size_t size);
void alloc_free();

void* malloc(size_t bytes);
void* realloc(void* ptr, size_t new_size);
void* calloc(size_t num, size_t size);
void free(void* ptr);

#endif
