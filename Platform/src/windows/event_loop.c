#ifdef _WIN32

#include <windows.h>

#include "event_loop.h"

void poll_events(PlatformWindow window){
	
	HWND hWnd = get_window_hwnd(window);

	MSG msg;
	BOOL res = PeekMessage(&msg, 
						   0, 
						   0, 
						   0, 
						   TRUE);

	if (res) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

#endif