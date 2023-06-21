#ifndef __RECT__
#define __RECT__

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;
}Rect;

/*
void normalize_rect(Rect rect,
					int source_width,
					int source_height,
					Rect* out,
					int dest_width,
					int dest_height);

#ifdef _WIN32

#include <Windows.h>

void from_win32_rect(Rect* out, RECT rect);
void to_win32_rect(RECT* out, Rect rect);

#endif
*/


bool colide(
	Rect* rect1, 
	Rect* rect2);
#endif