#ifndef KERNEL_ALLOCATOR_H
#define KERNEL_ALLOCATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>

/**
 * ALIGN: Fundamental alignment for memory allocator structures.
 * 
 * Uses alignof(max_align_t) which represents the alignment of the most
 * strictly aligned fundamental type. This is guaranteed to be suitable
 * for any scalar type and provides optimal performance across platforms.
 * 
 * On RP2350 (32-bit ARM Cortex-M33): typically 8 bytes
 * On 64-bit systems: typically 16 bytes
 */
#define ALIGN alignof(max_align_t)

typedef struct MemoryHeader MemoryHeader_t;
/**
 * Memory block header structure.
 * 
 * The compiler automatically adds padding between in_use and next to ensure
 * proper pointer alignment (4-byte on 32-bit ARM). We explicitly request
 * max_align_t alignment for optimal performance and portability using C11
 * standard alignas on the first member.
 * 
 * The alignment of a struct is determined by its most strictly aligned member.
 * By aligning the first member to max_align_t, we ensure the entire struct
 * (and thus all instances) are properly aligned for optimal performance.
 * 
 * Layout on 32-bit ARM (ALIGN=8):
 *   size_t size;        [4 bytes, offset 0]
 *   bool in_use;        [1 byte,  offset 4]
 *   [padding]           [3 bytes, offset 5-7] (automatic)
 *   MemoryHeader_t* next; [4 bytes, offset 8]
 *   [padding to ALIGN]  [4 bytes, offset 12-15] (for max_align_t)
 * Total: 16 bytes (with ALIGN=8)
 */
struct MemoryHeader {
	alignas(ALIGN) size_t size;
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
