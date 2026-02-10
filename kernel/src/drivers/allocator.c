#include "allocator.h"

static uint8_t* _heap_start = NULL;
static size_t _heap_size;

static MemoryHeader_t* _heap_first = NULL;
static MemoryHeader_t* _heap_last = NULL;

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

static uintptr_t _current_heap_end() {
	return (uintptr_t)_heap_last + sizeof(MemoryHeader_t) + _heap_last->size;
}

static void _defragment_address(MemoryHeader_t* header) {
	if (header == NULL || header->in_use) return;
	if (header->next == NULL) return;

	MemoryHeader_t* next = header->next;
	while (next->next != NULL && !next->next->in_use) {
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
void alloc_init(uint8_t* heap_start, size_t size) {
	// just to filter out basic errors
	if (heap_start == NULL) return;

	uintptr_t aligned_start = ((uintptr_t)heap_start + (ALIGN - 1)) & ~(ALIGN - 1);
	uintptr_t alignment_loss = aligned_start - (uintptr_t)heap_start;

	if (size < alignment_loss + sizeof(MemoryHeader_t) * 4) return;

	_heap_start = aligned_start;
	_heap_size = size - alignment_loss;

	_heap_first = (MemoryHeader_t*)_heap_start;
	_heap_first->size = 0;
	_heap_first->in_use = true;
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
void* malloc(size_t bytes) {
	if (_heap_start == NULL || bytes == 0) return NULL;

	// make sure bytes is aligned (round up)
	bytes = (bytes + (ALIGN - 1)) & ~(ALIGN - 1);

	// check if the last block is free, if so we can extend it
	if (!_heap_last->in_use) {
		if (_heap_last->size >= bytes) {
			_heap_last->size = bytes;
			return (void*)(_heap_last + 1);
		}

		uintptr_t space_needed = bytes - _heap_last->size;
		uintptr_t heap_end = _current_heap_end();
		if (space_needed > SIZE_MAX - heap_end) return NULL;

		// make sure there's enough space to extend
		if (heap_end + space_needed
				<= (uintptr_t)_heap_start + (uintptr_t)_heap_size) {
			_heap_last->size = bytes;
			return (void*)(_heap_last + 1);
		}

		// otherwise just fall through and continue as normal
	}

	// otherwise check if we have room to allocate memory
	// at the end of the currently used heap
	// (quicker than searching, so this is prioritised)
	uintptr_t heap_end = _current_heap_end();
	size_t new_block_size = sizeof(MemoryHeader_t) + bytes;
	if (new_block_size > SIZE_MAX - heap_end) return NULL;

	if (heap_end + new_block_size
			<= (uintptr_t)_heap_start + (uintptr_t)_heap_size) {
		_heap_last->next = (MemoryHeader_t*)(
			(uint8_t*)_heap_last
			+ _heap_last->size
			+ sizeof(MemoryHeader_t)
		);

		_heap_last = _heap_last->next;

		_heap_last->size = bytes;
		_heap_last->in_use = true;
		_heap_last->next = NULL;

		return (void*)(_heap_last + 1);
	}

	// search for an unused block
	MemoryHeader_t* search = _heap_first;
	while (search != NULL &&
			(search->in_use ||
			bytes > search->size)) {
		// if possible, defragment
		MemoryHeader_t* next = search->next;
		if (next != NULL && !search->in_use && !next->in_use) {
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
	
	search->in_use = true;

	// already correct size
	if (search->size == bytes) {
		return (void*)(search + 1);
	}

	// if the remaining space isn't even big enough for another header
	// + 4 bytes of data just consume the extra space
	size_t remaining_space = search->size - bytes;
	if (remaining_space < sizeof(MemoryHeader_t) + 4) {
		return (void*)(search + 1);
	}

	// otherwise fragment to leave extra space free
	MemoryHeader_t* fragment =
		(MemoryHeader_t*)((uint8_t*)search + sizeof(MemoryHeader_t) + bytes);
	fragment->size = remaining_space - sizeof(MemoryHeader_t);
	fragment->in_use = false;
	fragment->next = search->next;
	if (fragment->next == NULL) _heap_last = fragment;
	
	search->size = bytes;
	search->next = fragment;

	return (void*)(search + 1);
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
void* realloc(void* ptr, size_t new_size) {
	uint8_t* buffer = malloc(new_size);

	if (ptr == NULL || buffer == NULL) return buffer;

	MemoryHeader_t* old_header = _get_header(ptr);

	// copy old data to new buffer
	size_t memory_to_copy =
		new_size > old_header->size ? old_header->size : new_size;
	for (size_t i = 0; i < memory_to_copy; i++) {
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
void* calloc(size_t num, size_t size) {
	if (num == 0 || size == 0) return NULL;

	// prevent theoretical overflows (although there isn't anywhere near
	// enough memory so if you're overflowing something is very wrong)
	if (num > SIZE_MAX / size) {
		return NULL;
	}

	size_t total_bytes = num * size;

	uint8_t* buffer = (uint8_t*)malloc(total_bytes);
	if (buffer == NULL) return NULL;

	// zero the memory
	for (size_t i = 0; i < total_bytes; i++) {
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
	header->in_use = false;

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
