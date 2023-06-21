#ifndef __MEMORY__UTILS__
#define __MEMORY__UTILS__

#include <stdint.h>

void* safe_malloc(size_t size);
void* safe_realoc(void* old, size_t size);

#endif