#ifndef __WINDOW__
#define __WINDOW__

#include <stdbool.h>
#include <wchar.h>

#include <rect.h>

typedef int PlatformWindow;

typedef void(*ShutDownProc)();
typedef void(*OnEnterExitControlMode)(bool b);

typedef struct {
	ShutDownProc shut_down_proc;
	OnEnterExitControlMode on_enter_exit_control_mode;

	int width;
	int height;
	const wchar_t* window_name;
} WindowStruct;

void init_windows_system();

bool create_window(WindowStruct* window_struct,
				   PlatformWindow* out,
				   bool visible);

void show_window(PlatformWindow window);

void draw_to_window(PlatformWindow window,
					int bit_count,
				    char* buffer,
					Rect rect);

void get_window_rect(
	PlatformWindow window, 
	Rect* rect);

void get_client_rect(
	PlatformWindow window,
	Rect* rect
);

#ifdef _WIN32

#include <Windows.h>

HWND get_window_hwnd(PlatformWindow window);

#endif


#endif