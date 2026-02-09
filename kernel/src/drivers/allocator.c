#include "allocator.h"

static uint8_t* __heap_start = NULL;
static size_t __heap_size;

MemoryHeader_t* __get_header(void* ptr) {
	return (MemoryHeader_t*)ptr - 1;
}

void alloc_init(uint8_t* heap_start, size_t size) {
	__heap_start = heap_start;
	__heap_size = size;

	__heap_first = (MemoryHeader_t*)__heap_start;
	__heap_first->size = 0;
	__heap_first->in_use = true;
	__heap_first->next = NULL;
	__heap_last = __heap_first;
}

void alloc_free() {
	// free the heap
	MemoryHeader_t* header = __heap_first;
	while (header != NULL) {
		MemoryHeader_t* next = header->next;
		free(header);
	}

	__heap_start = NULL;
	__heap_size = 0;
	__heap_first = NULL;
	__heap_last = NULL;
}

void* malloc(size_t bytes) {
	if (__heap_start == NULL) return NULL;

	// check if we have room to allocate memory
	// at the end of the currently used heap
	if ((uint8_t*)__heap_last + __heap_last->size
			- __heap_start + bytes < __heap_size) {
		__heap_last = (MemoryHeader_t*)(
			(uint8_t*)__heap_last +
			__heap_last->size +
			sizeof(MemoryHeader_t)
		);

		__heap_last->size = bytes;
		__heap_last->in_use = true;
		__heap_last->next = NULL;

		return (void*)(__heap_last + 1);
	}

	// search for an unused block
	MemoryHeader_t* search = __heap_first;
	while (search != NULL &&
			(search->in_use ||
			bytes < search->size)) {
		// if possible, defragment
		MemoryHeader_t* next = search->next;
		if (!search->in_use && !next->in_use) {
			search->size += next->size + sizeof(MemoryHeader_t);
			search->next = next->next;

			// if the defragmented block is now
			if (bytes < search->size) break;
		}

		search = search->next;
	}

	// no free memory
	if (search == NULL) {
		return NULL;
	}

	// if it is the correct size
	if (search->size == bytes) {
		search->in_use = true;
		return (void*)(search + 1);
	}

	// fragment to leave extra space free
	size_t remaining_space = search->size - bytes;
	if (remaining_space < sizeof(MemoryHeader_t)) {
		return (void*)(search + 1);
	}

	MemoryHeader_t* fragment = (MemoryHeader_t*)((uint8_t*)search + search->size + sizeof(MemoryHeader_t));
	fragment->size = remaining_space - sizeof(MemoryHeader_t);
	fragment->next = search->next;
	search->next = fragment;

	return (void*)(search + 1);
}

void* realloc(void* ptr, size_t new_size) {
	uint8_t* buffer = malloc(new_size);

	MemoryHeader_t* old_header = __get_header(ptr);

	// copy old data to new buffer
	size_t memory_to_copy = new_size > old_header->size ? old_header->size : new_size;
	for (size_t i = 0; i < memory_to_copy; i++) {
		buffer[i] = ((uint8_t*)ptr)[i];
	}

	free(ptr);

	return buffer;
}

void* calloc(size_t num, size_t size) {
	if (__heap_start == NULL) return NULL;

	size_t total_bytes = num * size;

	MemoryHeader_t* buffer = malloc(total_bytes);
	// zero the memory
	for (size_t i = 0; i < total_bytes; i++) {
		*(((uint8_t*)buffer) + i) = 0;
	}

	return buffer;
}

void free(void* ptr) {
	__get_header(ptr)->in_use = false;
}

void __defragment() {
	MemoryHeader_t* current = __heap_first;
	MemoryHeader_t* next = current->next;
	while (current->next != NULL) {
		if (!current->in_use && !next->in_use) {
			current->size += next->size + sizeof(MemoryHeader_t);
			current->next = next->next;
			// continue in case the current block can be
			// defragmented with the new next block
			continue;
		}
		current = next->next;
	}
}
