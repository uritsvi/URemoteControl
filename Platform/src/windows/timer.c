#ifdef _WIN32

#include <Windows.h>

#include "timer.h"

void set_timer(
	TimerProc callback, 
	uint32_t ms) {

	SetTimer(
		NULL, 
		0, 
		ms, 
		callback);
}

#endif