#ifndef __ZERO__MEMORY__
#define __ZERO__MEMORY__

#include <memory.h>

#ifndef ZeroMemory
	#define ZeroMemory(x, size) memset(x, 0, size);
#endif

#endif