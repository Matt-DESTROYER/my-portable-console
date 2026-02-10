#ifndef KERNEL_ALLOCATOR_H
#define KERNEL_ALLOCATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>

#define ALIGN alignof(max_align_t)
#define WORD_SIZE sizeof(size_t)

#define MINIMUM_HEAP_SIZE  4
#define MINIMUM_BLOCK_SIZE 4

typedef struct MemoryHeader {
	// alignas forces start address to be at an ALIGN byte boundary
	// and the total struct size to be a multiple of ALIGN
	alignas(ALIGN) uintptr_t size;
	struct MemoryHeader* next;
	uint8_t freed;
} MemoryHeader_t;

void alloc_init(uint8_t* heap_start, uintptr_t size);
void alloc_free();

void* malloc(uintptr_t bytes);
void* realloc(void* ptr, uintptr_t new_size);
void* calloc(uintptr_t num, uintptr_t size);
void free(void* ptr);

void* memcpy(void* restrict dest, const void* restrict src, uintptr_t n);

#endif
