#ifndef __TIMER__
#define __TIMER__

#include <stdint.h>

typedef void(*TimerProc)();

void set_timer(
	TimerProc callback, 
	uint32_t ms);

#endif