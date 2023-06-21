#ifdef _WIN32

#include <Windows.h>

#include <platfrom_memory_utils.h>
#include <Common.h>

#include <stdio.h>

#include "window.h"
#include "platform_config.h"
#include "input.h"

#define ENTER_EXIT_MENUE_BAR 0

#define WINDOW_STYLE_OVERLAPED_NO_RESIZE (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)
#define WINDOW_STYLE_POPUP_AND_DRAG (WS_MINIMIZEBOX | WS_SYSMENU)

#define SELECT_TARGET_MONITOR_WINDOW_WIDTH 300
#define SELECT_TARGET_MONITOR_WINDOW_HEIGHT 300

typedef struct {
	HWND hWnd;
	WindowStruct* window_struct;
	int window_border_height;

} WIN32Window;

static const wchar_t* g_WindowClassName = L"PlatformWindow";
static WIN32Window* g_Windows[MAX_OBJECTS];
static int g_NextWindowIndex;

static LONG g_LastWindowStyle;

WIN32Window* _create_win32Window(HWND hWnd, 
								 HWND toolbarHWND,
								 WindowStruct* window_struct) {
	
	WIN32Window* win32_window = 
		safe_malloc(sizeof(WIN32Window));

	win32_window->hWnd = hWnd;
	win32_window->window_struct = window_struct;

	return win32_window;
}

LRESULT SetTargetMonitorWindowWndProc(
	HWND hWnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam) {
	
	return DefWindowProc(
		hWnd,
		msg,
		wParam,
		lParam);
}

int _calculate_border_size(HWND hwnd) {
	RECT windowRect;
	RECT clientRect;

	GetWindowRect(hwnd, &windowRect);

	GetClientRect(hwnd, &clientRect);

	int border_height = 
		(windowRect.bottom - windowRect.top) - 
		(clientRect.bottom - clientRect.top);

	return border_height;
}

void _add_title_bar_size_to_window_size(
	HWND hWnd,
	bool b) {
		
	WIN32Window* win32_window = 
		GetWindowLongPtrA(hWnd, GWLP_USERDATA);

	int height = win32_window->window_border_height;

	if (!b) {
		height = -height;
	}

	RECT window_rect;
	GetWindowRect(hWnd, &window_rect);

	SetWindowPos(
		hWnd, 
		NULL, 
		window_rect.left, 
		window_rect.top, 
		(window_rect.right - window_rect.left),
		(window_rect.bottom - window_rect.top) + height,
		SWP_SHOWWINDOW);
}

LRESULT WndProc(HWND hWnd, 
				UINT msg, 
				WPARAM wParam, 
				LPARAM lParam) {

	switch (msg) {
		case WM_CLOSE: {
			WIN32Window* win32_window =
				GetWindowLongPtr(hWnd, GWLP_USERDATA);

			if (win32_window != NULL) {
				ShutDownProc shut_down_proc =
					win32_window->window_struct->shut_down_proc;
				
				if (shut_down_proc != NULL) {
					shut_down_proc();
				}
			}

			PostQuitMessage(0);
			return 0;
		}break;
	
		
		case WM_SETFOCUS:{
			WMFocuse_callback(hWnd);
		}break;

		case WM_KILLFOCUS: {
			WMKillFocuse_callback(hWnd);
		}break;
		
			
		case WM_HOTKEY: {
			if (wParam == ENTER_EXIT_MENUE_BAR) {
				
				LONG g_NewStyle = g_LastWindowStyle;

				SetWindowLongA(
					hWnd,
					GWL_STYLE,
					g_LastWindowStyle | WS_VISIBLE);
					
			

				if (g_NewStyle == WINDOW_STYLE_OVERLAPED_NO_RESIZE) {
					WMKillFocuse_callback(hWnd);
					EnterControlMode();

					g_LastWindowStyle = WS_POPUPWINDOW;

					//_add_title_bar_size_to_window_size(hWnd, true);


					WIN32Window* win32_window = 
						GetWindowLongPtr(
							hWnd, 
							GWLP_USERDATA);

					OnEnterExitControlMode callback = 
						win32_window->window_struct->on_enter_exit_control_mode;

					if (callback != NULL) {
						callback(true);
					}

				}
				else if(g_NewStyle == WS_POPUPWINDOW) {
					WMFocuse_callback(hWnd);
					ExitControlMode();

					g_LastWindowStyle = WINDOW_STYLE_OVERLAPED_NO_RESIZE;

					//_add_title_bar_size_to_window_size(hWnd, false);

					
					WIN32Window* win32_window =
						GetWindowLongPtr(
							hWnd,
							GWLP_USERDATA);

					OnEnterExitControlMode callback =
						win32_window->window_struct->on_enter_exit_control_mode;

					if (callback != NULL) {
						callback(false);
					}

				}

		

			}
		}break;
		
	}
	return DefWindowProc(
		hWnd, 
		msg, 
		wParam, 
		lParam);

}

void get_window_rect(
	PlatformWindow window,
	Rect* rect) {
	WIN32Window* win32_window = g_Windows[window];

	GetWindowRect(
		win32_window->hWnd, 
		rect);
}

void get_client_rect(
	PlatformWindow window,
	Rect* rect
) {
	WIN32Window* win32_window = g_Windows[window];

	GetClientRect(
		win32_window->hWnd,
		rect);
}

void init_windows_system() {
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(wc));

	PlatformConfig* config = get_platform_config();

	wc.cbSize = sizeof(wc);
	wc.hInstance = config->hInstance;
	wc.lpszClassName = g_WindowClassName;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;

	RegisterClassEx(&wc);
}

bool _register_hot_key(HWND parent) {
	BOOL res = RegisterHotKey(
		parent,
		ENTER_EXIT_MENUE_BAR,
		MOD_CONTROL | MOD_NOREPEAT,
		VK_SPACE);
	return true;
}

bool create_window(WindowStruct* window_struct,
				   PlatformWindow* out,
				   bool visible) {

	PlatformConfig* config = get_platform_config();

	int screen_x = GetSystemMetrics(SM_CXSCREEN) - window_struct->width;
	int screen_y = GetSystemMetrics(SM_CYSCREEN) - window_struct->height;

	HWND hWnd = CreateWindowEx(NULL, 
				   g_WindowClassName, 
				   window_struct->window_name,
				   WINDOW_STYLE_OVERLAPED_NO_RESIZE,
				   screen_x / 2, 
				   screen_y / 2, 
				   window_struct->width,
				   window_struct->height,
				   NULL, 
				   NULL, 
				   config->hInstance, 
				   NULL);

	
	if (hWnd == NULL) {
		return false;
	}

	g_LastWindowStyle =
		WINDOW_STYLE_OVERLAPED_NO_RESIZE;


	HWND toolbarHWND = NULL;

	if (visible) {

		// handeling the error rase an 
		// internal compiler error on my system
		_register_hot_key(hWnd);
	}

	WIN32Window* win32_window = 
		_create_win32Window(
			hWnd, 
			toolbarHWND, 
			window_struct);

	SetWindowLongPtr(hWnd,
		GWLP_USERDATA,
		win32_window);

	*out = g_NextWindowIndex++;
	g_Windows[*out] = win32_window;

	return true;
}

void show_window(PlatformWindow window) {
	WIN32Window* win32_window = 
		g_Windows[window];

	PlatformConfig* config = get_platform_config();

	ShowWindow(win32_window->hWnd, 
			   config->cmdShow);

	win32_window->window_border_height
		= _calculate_border_size(win32_window->hWnd);

	SetWindowLongA(
		win32_window->hWnd,
		GWL_STYLE,
		WS_POPUPWINDOW | WS_VISIBLE);



}

void draw_to_window(PlatformWindow window,
					int bit_cound,
					char* buffer,
					Rect rect) {


	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	WIN32Window* WIN32Window = g_Windows[window];

	BITMAPINFOHEADER bi;
	ZeroMemory(&bi, sizeof(bi));

	bi.biSize = sizeof(BITMAPINFOHEADER);;
	bi.biWidth = width;
	bi.biHeight = -height;
	bi.biBitCount = bit_cound;
	bi.biCompression = BI_RGB;
	bi.biPlanes = 1;


	HDC window_dc = GetDC(WIN32Window->hWnd);

	
	SetDIBitsToDevice(window_dc,
		rect.left, 
		rect.top,
		width - 1, 
		height,
		0,
		0,
		0,
		height,
		buffer, 
		&bi, 
		DIB_RGB_COLORS);



}

#ifdef _WIN32

HWND get_window_hwnd(PlatformWindow window) {
	return g_Windows[window]->hWnd;
}
#endif


#endif