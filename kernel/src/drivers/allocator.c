#include "allocator.h"

#ifdef DEBUG
#include <stdlib.h>
#include <stdio.h>
#endif

static uint8_t* _heap_start = NULL;
static uintptr_t _heap_size;

static MemoryHeader_t* _heap_first = NULL;
static MemoryHeader_t* _heap_last = NULL;

static uintptr_t _align(uintptr_t size, uintptr_t alignment) {
	return (size + (alignment - 1)) & ~(alignment - 1);
}

/**
 * Get the memory block header corresponding to a data pointer.
 * @param ptr Data pointer that was returned to the caller (points to the
 * payload area).
 * @returns Pointer to the MemoryHeader_t stored immediately before `ptr`.
 * If `ptr` is NULL or not a pointer returned by this allocator, the behavior
 * is undefined.
 */
static MemoryHeader_t* _get_header(void* ptr) {
	return (MemoryHeader_t*)ptr - 1;
}

static bool _is_free(MemoryHeader_t* header) {
	return header->freed != 0;
}

static void* _get_buffer_start(MemoryHeader_t* header) {
	return (void*)(header + 1);
}

static uintptr_t _current_heap_end() {
	if (_heap_last == NULL) return 0;

	return (uintptr_t)_heap_last + sizeof(MemoryHeader_t) + _heap_last->size;
}

static void _extend_address(MemoryHeader_t* header) {
	if (header == NULL) return;
	if (header->next == NULL || !_is_free(header->next)) return;

	MemoryHeader_t* next = header->next;
	while (next->next != NULL && _is_free(next->next)) {
		next = next->next;
	}

	if (next->next == NULL) {
		// this has to be the end, otherwise something has gone very wrong...
		header->next = NULL;
		_heap_last = header;
		// dropping extra headers is intentional, it enables avoiding a search
		// in malloc and just appending to the end of the heap
	} else {
		header->size = (uintptr_t)next->next
			- (uintptr_t)header
			- sizeof(MemoryHeader_t);
		header->next = next->next;
	}

	return;
}

static void _defragment_address(MemoryHeader_t* header) {
	if (header == NULL || !_is_free(header)) return;
	if (header->next == NULL) {
		_heap_last = header;
		return;
	}
	_extend_address(header);
}

/**
 * Initialize the allocator to manage a contiguous heap region.
 *
 * Sets the internal heap base and size and installs an initial sentinel
 * header at the start of the region which serves as the first list node for
 * allocations.
 *
 * @param heap_start Pointer to the start of the heap memory region to manage.
 * @param size Size of the heap region in bytes.
 */
void alloc_init(uint8_t* heap_start, uintptr_t size) {
	// just to filter out basic errors
	if (heap_start == NULL) return;

	uintptr_t aligned_start = _align((uintptr_t)heap_start, ALIGN);
	uintptr_t alignment_loss = aligned_start - (uintptr_t)heap_start;

	if (size < alignment_loss + sizeof(MemoryHeader_t) + MINIMUM_HEAP_SIZE) return;

	_heap_start = (uint8_t*)aligned_start;
	_heap_size = size - alignment_loss;

	_heap_first = (MemoryHeader_t*)_heap_start;
	_heap_first->size = 0;
	_heap_first->freed = 0;
	_heap_first->next = NULL;
	_heap_last = _heap_first;
}

/**
 * Release all allocated heap blocks and reset the allocator state.
 *
 * Frees every MemoryHeader_t in the managed heap and sets internal allocator
 * globals (_heap_start, _heap_size, _heap_first, _heap_last) to indicate
 * the heap is uninitialized. After this call, allocations will fail until
 * alloc_init is called again.
 */
void alloc_free() {
	_heap_start = NULL;
	_heap_size = 0;
	_heap_first = NULL;
	_heap_last = NULL;
}

/**
 * Allocate a contiguous block of memory from the custom heap.
 *
 * Allocates a block of at least `bytes` bytes from the allocator's managed
 * heap and returns a pointer to the start of the usable memory region.
 * @param bytes Number of bytes requested.
 * @return Pointer to the allocated memory region, or NULL if the heap is
 * uninitialized or no suitable block is available.
 */
void* malloc(uintptr_t bytes) {
	if (_heap_start == NULL || bytes == 0) return NULL;

	// make sure bytes is aligned (round up)
	bytes = _align(bytes, ALIGN);

	// check if the last block is free, if so we can extend it
	if (_is_free(_heap_last)) {
		if (_heap_last->size >= bytes) {
			_heap_last->size = bytes;
			_heap_last->freed = 0;
			return _get_buffer_start(_heap_last);
		}

		uintptr_t space_needed = bytes - _heap_last->size;
		uintptr_t heap_end = _current_heap_end();
		if (space_needed > UINTPTR_MAX - heap_end) return NULL;

		// make sure there's enough space to extend
		if (heap_end + space_needed
				<= (uintptr_t)_heap_start + _heap_size) {
			_heap_last->size = bytes;
			_heap_last->freed = 0;
			return _get_buffer_start(_heap_last);
		}

		// otherwise just fall through and continue as normal
	}

	// otherwise check if we have room to allocate memory
	// at the end of the currently used heap
	// (quicker than searching, so this is prioritised)
	uintptr_t heap_end = _current_heap_end();
	uintptr_t new_block_size = sizeof(MemoryHeader_t) + bytes;
	if (new_block_size > UINTPTR_MAX - heap_end) return NULL;

	if (heap_end + new_block_size
			<= (uintptr_t)_heap_start + _heap_size) {
		_heap_last->next = (MemoryHeader_t*)(
			(uint8_t*)_heap_last
			+ _heap_last->size
			+ sizeof(MemoryHeader_t)
		);

		_heap_last = _heap_last->next;

		_heap_last->size = bytes;
		_heap_last->freed = 0;
		_heap_last->next = NULL;

		return _get_buffer_start(_heap_last);
	}

	// search for an unused block
	MemoryHeader_t* search = _heap_first;
	while (search != NULL &&
			(!_is_free(search) ||
			bytes > search->size)) {
		// if possible, defragment
		MemoryHeader_t* next = search->next;
		if (next != NULL && _is_free(search) && _is_free(next)) {
			_defragment_address(search);

			// if the defragmented block is now large enough
			if (bytes <= search->size) break;
		}

		search = search->next;
	}

	// no free memory
	if (search == NULL) {
		return NULL;
	}
	
	search->freed = 0;

	// already correct size
	if (search->size == bytes) {
		return _get_buffer_start(search);
	}

	// if the remaining space isn't even big enough for another header
	// + `MINIMUM_HEAP_SIZE` bytes of data just consume the extra space
	uintptr_t remaining_space = search->size - bytes;
	if (remaining_space < sizeof(MemoryHeader_t) + MINIMUM_HEAP_SIZE) {
		return _get_buffer_start(search);
	}

	// otherwise fragment to leave extra space free
	MemoryHeader_t* fragment =
		(MemoryHeader_t*)((uint8_t*)search + sizeof(MemoryHeader_t) + bytes);
	fragment->size = remaining_space - sizeof(MemoryHeader_t);
	fragment->next = search->next;
	free(_get_buffer_start(fragment));
	
	search->size = bytes;
	search->next = fragment;

	return _get_buffer_start(search);
}

/**
 * Resize an allocated memory block to a new size, preserving existing data up
 * to the smaller of the old and new sizes.
 * @param ptr Pointer to a previously allocated memory block (must be non-NULL
 * and returned by this allocator).
 * @param new_size New size in bytes for the allocation.
 * @returns Pointer to the newly allocated memory containing the copied data,
 * or `NULL` if allocation failed.
 */
void* realloc(void* ptr, uintptr_t new_size) {
	if (ptr == NULL) return malloc(new_size);
	if (new_size == 0) {
		free(ptr);
		return NULL;
	}

	new_size = _align(new_size, ALIGN);

	MemoryHeader_t* old_header = _get_header(ptr);
	// extend in case that gives us the space we need
	_extend_address(old_header);
	if (old_header->size >= new_size) {

		// TODO: re-fragment extra space

		return _get_buffer_start(old_header);
	}

	uint8_t* buffer = malloc(new_size);
	if (buffer == NULL) return NULL;

	// copy old data to new buffer
	uintptr_t copy_size = old_header->size;
	if (copy_size > new_size) copy_size = new_size;

	for (uintptr_t i = 0; i < copy_size; i++) {
		buffer[i] = ((uint8_t*)ptr)[i];
	}

	free(ptr);

	return buffer;
}

/**
 * Allocate memory for an array of elements and initialize all bytes to zero.
 *
 * @param num Number of elements to allocate.
 * @param size Size in bytes of each element.
 * @returns Pointer to the allocated, zero-initialized memory, or `NULL` if
 * the allocator is uninitialized or allocation fails.
 */
void* calloc(uintptr_t num, uintptr_t size) {
	if (num == 0 || size == 0) return NULL;

	// prevent theoretical overflows (although there isn't anywhere near
	// enough memory so if you're overflowing something is very wrong)
	if (num > UINTPTR_MAX / size) {
		return NULL;
	}

	uintptr_t total_bytes = num * size;

	uint8_t* buffer = (uint8_t*)malloc(total_bytes);
	if (buffer == NULL) return NULL;

	// zero the memory
	for (uintptr_t i = 0; i < total_bytes; i++) {
		buffer[i] = 0;
	}

	return (void*)buffer;
}

/**
 * Mark the memory block referenced by `ptr` as free.
 *
 * @param ptr Pointer to a data region previously returned by `malloc`,
 * `calloc`, or `realloc`. Behavior is undefined if `ptr` is `NULL` or was not
 * allocated by this allocator.
 */
void free(void* ptr) {
	if (ptr == NULL) return;
	
	MemoryHeader_t* header = _get_header(ptr);
	if (header->freed > 0) {
#ifdef DEBUG
		fprintf(stderr, "Error: double free detected at %p\n", ptr);
		abort();
#endif
		return;
	}

	if (header->freed < UINT8_MAX) {
		header->freed++;
	}

	_defragment_address(header);
}

/**
 * Coalesces adjacent free blocks in the allocator's heap to reduce
 * fragmentation.
 *
 * Traverses the allocator's header list starting at _heap_first and merges
 * consecutive blocks that are not in use by increasing the earlier block's
 * size and updating next pointers; modifies the allocator's heap metadata.
 */
static void _defragment_all() {
	if (_heap_first == NULL) return;

	MemoryHeader_t* current = _heap_first;
	while (current->next != NULL) {
		_defragment_address(current);
		current = current->next;
	}
}
