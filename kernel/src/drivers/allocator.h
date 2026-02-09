#ifndef KERNEL_ALLOCATOR_H
#define KERNEL_ALLOCATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct MemoryHeader MemoryHeader_t;
struct MemoryHeader {
	size_t size;
	bool in_use;
	MemoryHeader_t* next;
};

void alloc_init(uint8_t* heap_start, size_t size);
void alloc_free();

void* malloc(size_t bytes);
void* realloc(void* ptr, size_t new_size);
void* calloc(size_t num, size_t size);
void free(void* ptr);

#endif
