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

/**
 * Compute the address immediately after the last block's usable data in the managed heap.
 *
 * @returns The byte address (as `uintptr_t`) one past the last block's data region; `0` if the allocator is uninitialized (`_heap_last` is `NULL`).
 */
static uintptr_t _current_heap_end() {
	if (_heap_last == NULL) return 0;

	return (uintptr_t)_heap_last + sizeof(MemoryHeader_t) + _heap_last->size;
}

/**
 * Merge consecutive free blocks after `header` into `header`, enlarging its usable size.
 *
 * If the merged region reaches the end of the managed heap, `header->next` is set to NULL
 * and the global `_heap_last` is updated to point to `header`. If a heap corruption is
 * detected (invalid pointer ordering), a diagnostic is emitted and the process aborts in
 * debug builds.
 *
 * @param header Header whose following free blocks should be absorbed; no action is taken if `header` is NULL or its immediate successor is not free.
 */
static void _extend_block(MemoryHeader_t* header) {
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
		if ((uintptr_t)next->next <= (uintptr_t)header) {
			// this should never happen, corrupted heap...

			// you fucked veeeeery up bad if you got here
			// that or i've done something really dumb

#ifdef DEBUG
			fprintf(stderr, "Error: corrupted heap detected\n");
			abort();
#endif

			// TODO: figure out what behaviour should be in release

			return;
		}
		header->size = (uintptr_t)next->next
			- (uintptr_t)header
			- sizeof(MemoryHeader_t);
		header->next = next->next;
	}

	return;
}

/**
 * Ensure a free block is coalesced with adjacent free blocks and that the heap end sentinel is correct.
 *
 * If `header` refers to a free block, this will merge it with any following free blocks and update
 * the allocator's end-of-heap marker when `header` becomes the last block. If `header` is NULL or
 * not free, the function does nothing.
 *
 * @param header Pointer to the block header to defragment (must belong to this allocator).
 */
static void _defragment_block(MemoryHeader_t* header) {
	if (header == NULL || !_is_free(header)) return;
	if (header->next == NULL) {
		_heap_last = header;
		return;
	}
	_extend_block(header);
}

/**
 * Split a free memory block into a leading block of the requested size and a trailing free fragment when there is sufficient leftover space.
 *
 * If the header's size minus `size` is smaller than the space required for a new MemoryHeader_t plus MINIMUM_BLOCK_SIZE, no action is taken.
 *
 * @param header Pointer to the free block's header to be potentially split.
 * @param size Requested size (in bytes) for the leading block; remaining bytes (if large enough) form a new free fragment.
 */
static void _fragment_block(MemoryHeader_t* header, uintptr_t size) {
	uintptr_t remaining_space = header->size - size;
	// if there isn't enough space for anything really
	if (remaining_space < sizeof(MemoryHeader_t) + MINIMUM_BLOCK_SIZE) {
		return;
	}

	// otherwise fragment to leave extra space free
	MemoryHeader_t* fragment =
		(MemoryHeader_t*)((uint8_t*)header + sizeof(MemoryHeader_t) + size);
	fragment->size = remaining_space - sizeof(MemoryHeader_t);
	fragment->next = header->next;
	fragment->freed = 0;
	free(_get_buffer_start(fragment));
	
	header->size = size;
	header->next = fragment;
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
 * Requests a block of at least `bytes` bytes from the allocator and returns
 * a pointer to the start of the usable data region.
 * @param bytes Number of bytes requested; value is rounded up to the allocator's alignment (ALIGN). A request of 0 returns NULL.
 * @return Pointer to the start of the allocated usable memory, or NULL if the heap is uninitialized, `bytes` is zero, or no suitable block is available.
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
			_defragment_block(search);

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

	_fragment_block(search, bytes);

	return _get_buffer_start(search);
}

/**
 * Resize an allocated memory block to hold at least `new_size` bytes, preserving existing data up to the smaller of the old and new sizes.
 *
 * If `ptr` is `NULL`, the call is equivalent to `malloc(new_size)`. If `new_size` is 0, the allocation is freed and `NULL` is returned. The requested size is rounded up to the allocator's alignment before allocation.
 * @param ptr Pointer to a previously allocated memory block returned by this allocator, or `NULL`.
 * @param new_size Desired size in bytes for the allocation.
 * @returns Pointer to a memory region containing the original data (possibly relocated), or `NULL` if allocation failed or `new_size` was 0.
void* realloc(void* ptr, uintptr_t new_size) {
	if (ptr == NULL) return malloc(new_size);
	if (new_size == 0) {
		free(ptr);
		return NULL;
	}

	new_size = _align(new_size, ALIGN);

	MemoryHeader_t* old_header = _get_header(ptr);
	// extend in case that gives us the space we need
	_extend_block(old_header);
	if (old_header->size >= new_size) {
		_fragment_block(old_header, new_size);
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
 * Release a previously allocated memory block back to the allocator.
 *
 * If the allocator is uninitialized or `ptr` is `NULL`, the call is a no-op.
 * If the block has already been freed, a double-free is detected: in debug
 * builds this prints an error and aborts; otherwise the call returns without
 * modifying state. Otherwise the block's free counter is incremented (capped
 * at `UINT8_MAX`) and the allocator attempts to coalesce adjacent free blocks.
 *
 * @param ptr Pointer to a data region previously returned by `malloc`,
 *            `calloc`, or `realloc`. Behavior is undefined if `ptr` was not
 *            allocated by this allocator.
 */
void free(void* ptr) {
	if (_heap_start == NULL || ptr == NULL) return;
	
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

	_defragment_block(header);
}

/**
 * Coalesces adjacent free blocks in the allocator's heap to reduce fragmentation.
 *
 * Iterates headers from the first managed block and merges consecutive freed
 * blocks by increasing the earlier block's size and updating next pointers,
 * keeping the allocator's heap metadata consistent.
 */
static void _defragment_all() {
	if (_heap_first == NULL) return;

	MemoryHeader_t* current = _heap_first;
	while (current->next != NULL) {
		_defragment_block(current);
		current = current->next;
	}
}

void* memcpy(void* restrict dest, const void* restrict src, uintptr_t n) {
	uint8_t* _dest = (uint8_t*)dest;
	const uint8_t* _src = (const uint8_t*)src;

	for (uintptr_t i = 0; i < n; i++) {
		_dest[i] = _src[i];
	}

	return dest;
}