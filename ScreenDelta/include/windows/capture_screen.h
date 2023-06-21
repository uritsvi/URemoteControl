#ifndef __CAPTURE__SCREEN__
#define __CAPTURE__SCREEN__

#include <stdbool.h>

#include <rect.h>
#include <program_config.h>

#include "windows/monitor_info.h"

bool capture_screen(MonitorInfo* monitor,
					Rect* src_rect,
					Rect* target_rect,
					char* out_screen_buffer,
					int* out_size,
					ProgramConfig* program_config);

void bound_to_rect(
	RECT* src,
	RECT* dest,
	RECT bound_to,
	MonitorInfo* monitor,
	ProgramConfig* config);

void make_target(
	RECT* src,
	RECT* dest,
	RECT bound_to,
	MonitorInfo* monitor,
	ProgramConfig* config);

#endif