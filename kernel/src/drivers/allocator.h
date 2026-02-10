#ifndef KERNEL_ALLOCATOR_H
#define KERNEL_ALLOCATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>

#define ALIGN alignof(max_align_t)

typedef struct MemoryHeader {
	// alignas forces start address to be at an 8 byte boundary
	// and the total size to be a multiple of 8
	alignas(ALIGN) size_t size;
	struct MemoryHeader* next;
	bool in_use;
} MemoryHeader_t;

void alloc_init(uint8_t* heap_start, size_t size);
void alloc_free();

void* malloc(size_t bytes);
void* realloc(void* ptr, size_t new_size);
void* calloc(size_t num, size_t size);
void free(void* ptr);

#endif
