#include <stdlib.h>
#include <error.h>

#include "platfrom_memory_utils.h"

void* safe_malloc(size_t size) {
	void* mem = malloc(size);
	if (mem == NULL) {
		platform_exit_with_error("Failed to allocate memory to process\n");
		return NULL;
	}
	return mem;
}

void* safe_realoc(void* old, size_t size) {
	void* mem = realloc(old, size);
	if (mem == NULL) {
		platform_exit_with_error("Failed to reallocate memory to process\n");
		return NULL;
	}
	return mem;
}