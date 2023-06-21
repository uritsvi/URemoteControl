#ifndef __MONITORS__
#define __MONITORS__

#include <window.h>

typedef struct {
	int width;
	int height;

	int absulut_left;
	int absulut_top;

} PlatformMonitorInfo;


bool monitor_from_window(
	PlatformWindow window, 
	PlatformMonitorInfo* out);

bool monitor_from_index(
	int index,
	PlatformMonitorInfo* out
);

#endif