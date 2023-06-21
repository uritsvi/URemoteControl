#ifdef _WIN32

#include <Windows.h>

#include <math.h>

#include <Common.h>

#include <error.h>
#include <platform.h>
#include <stdio.h>

#include "windows/capture_screen.h"

#pragma comment(lib, "Helpers.lib")

typedef struct {
	MonitorInfo* monitor;
	Rect* src_rect;
	Rect* target_rect;
	char* out_screen_buffer;
	int* out_size;
	
	int target_index;
	int current_index;
	ProgramConfig* program_config;

	bool out_res;

} EnumMonitorInfo;

void make_target(
	Rect src,
	RECT* dest,
	RECT bound_to,
	MonitorInfo* monitor,
	ProgramConfig* config) {

	dest->left = ((float)src.left / (float)monitor->relativeRect.right) * (float)config->window_width;
	dest->right = ((float)src.right / (float)monitor->relativeRect.right) * (float)config->window_width;
	dest->top = ((float)src.top / (float)monitor->relativeRect.bottom) * (float)config->window_height;
	dest->bottom = ((float)src.bottom / (float)monitor->relativeRect.bottom) * (float)config->window_height;
}

void bound_to_rect(
	Rect src,
	RECT* dest,
	RECT bound_to,
	MonitorInfo* monitor,
	ProgramConfig* config) {
	
	src.left -= monitor->absulutRect.left;
	src.right -= monitor->absulutRect.left;
	src.top -= monitor->absulutRect.top;
	src.bottom -= monitor->absulutRect.top;

	make_target(
		src, 
		dest, 
		bound_to, 
		monitor, 
		config);
		
	dest->left = max(dest->left, bound_to.left);
	dest->right = min(dest->right, bound_to.right);
	dest->top = max(dest->top, bound_to.top);
	dest->bottom = min(dest->bottom, bound_to.bottom);
	

}

bool _capture_dc(
	MonitorInfo* monitor,
	Rect* src_rect,
	Rect* target_rect,
	char* out_screen_buffer,
	int* out_size,
	ProgramConfig* program_config,
	HDC dc) {



	RECT bound_to;
	build_target_rect(
		program_config, 
		&bound_to);

	bound_to_rect(
		*src_rect,
		target_rect,
		bound_to,
		monitor,
		program_config);
		
	int width = src_rect->right - src_rect->left;
	int height = src_rect->bottom - src_rect->top;

	int target_width = target_rect->right - target_rect->left;
	int target_height = target_rect->bottom - target_rect->top;


	HDC src_dc = dc;
	HDC streched_dc = CreateCompatibleDC(src_dc);
	HBITMAP streched_bitmap = CreateCompatibleBitmap(src_dc,
		target_width,
		target_height);

	SelectObject(streched_dc,
		streched_bitmap);

	SetStretchBltMode(streched_dc, HALFTONE);

	BOOL res =
		StretchBlt(streched_dc,
			0,
			0,
			target_width,
			target_height,
			dc,
			src_rect->left,
			src_rect->top,
			width,
			height,
			SRCCOPY);


	if (!res) {
		goto clean_up;
	}

	BITMAPINFOHEADER bi;
	ZeroMemory(&bi, sizeof(bi));


	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = target_width;
	bi.biHeight = -target_height;
	bi.biPlanes = 1;
	bi.biBitCount = program_config->target_bit_count;
	bi.biCompression = BI_RGB;

	res = GetDIBits(streched_dc,
		streched_bitmap,
		0,
		target_height,
		out_screen_buffer,
		&bi,
		DIB_RGB_COLORS);
	if (!res) {
		goto clean_up;
	}

	*out_size = bi.biSizeImage;

	target_rect->bottom = target_rect->top + (*out_size / target_width / 2);
	target_rect->right = target_rect->left + (*out_size / target_height / 2);

	int size = 
		(target_rect->right - target_rect->left) * 
		(target_rect->bottom - target_rect->top) * 2;

clean_up:
	ReleaseDC(GetDesktopWindow(), src_dc);
	
	DeleteDC(streched_dc);
	DeleteObject(streched_bitmap);

	return res;



}

bool _capture_screen_enum_proc(
	HMONITOR hMonitor, 
	HDC dc, 
	LPRECT rect, 
	LPARAM lParam) {

	EnumMonitorInfo* info = lParam;

	if (info->target_index != info->current_index ++) {
		return true;
	}

	info->out_res = _capture_dc(
		info->monitor,
		info->src_rect,
		info->target_rect,
		info->out_screen_buffer,
		info->out_size,
		info->program_config,
		dc);

	return false;

}

bool capture_screen(MonitorInfo* monitor,
					Rect* src_rect,
					Rect* target_rect,
					char* out_screen_buffer,
					int* out_size,
					ProgramConfig* program_config) {

	EnumMonitorInfo info;
	ZeroMemory(&info, sizeof(info));

	info.monitor = monitor;
	info.src_rect = src_rect;
	info.target_rect = target_rect;
	info.out_screen_buffer = out_screen_buffer;
	info.out_size = out_size;
	info.program_config = program_config;
	info.target_index = monitor->index;

	EnumDisplayMonitors(
		GetDC(NULL),
		NULL,
		_capture_screen_enum_proc,
		&info);

	return info.out_res;
}

#endif