#ifndef __MONITOR__INFO__
#define __MONITOR__INFO__

#include <window.h>

typedef struct {
	RECT absulutRect;
	RECT relativeRect;
	int requierd_buffer_size;
	int index;
}MonitorInfo;

#endif
