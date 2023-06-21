#ifdef _WIN32

#include <Windows.h>

#include <stdbool.h>
#include <stddef.h>
#include <math.h>

#include <double_buffers.h>
#include <platform.h>
#include <rect.h>
#include <input.h>
#include <platform.h>
#include <common.h>
#include <error.h>
#include <threads.h>
#include <platfrom_memory_utils.h>
#include <window.h>
#include <stdio.h>
#include <delta_finder.h>

#include "windows/monitor_info.h"
#include "windows/capture_screen.h"
#include "delta_finder.h"

#pragma comment(lib, "Platform.lib")
#pragma comment(lib, "Helpers.lib")


#define CAPTURE_ALL_SCREEN_EVENT 0
#define CAPTURE_ALL_SCREEN_TIMER_EVENT 1

#define CAPTURE_FULL_SCREEN_TIMEOUT_ENTERVAL 200

#define ON_DELTA_SENT 0x401

#define DRAGED_RECT_SIDE_ADDED_EDGE 50

typedef struct {
	int current_index;
	int target_index;

	MonitorInfo* out;
	bool res;
} EnumMonitorInfo;

static ProgramConfig* g_ProgramConfig;
static ScreenDeltaProcs* g_Procs;

static HWINEVENTHOOK g_Hook;

static HWND g_CurrentHWND;
static HWND g_MyWindow;

static bool g_IsDragging;
static bool g_Waiting;
static bool g_changeCurrentMonitor;

static DoubleBuffers* g_Buffers;

static MonitorInfo g_CurrentMonitorInfo;
static MonitorInfo g_NextMonitorInfo;

static int g_CurrentBufferSize;

static Mutex g_ChangeMonitorLock;

static RECT g_FocusedRect;

/*
* founds the monitor that requires the biggest buffer size to capture
* and return it's size
*/

BOOL _get_monitor_by_index_enum_proc(
	HMONITOR hMonitor, 
	HDC dc, 
	LPRECT rect, 
	LPARAM lParam) {

	EnumMonitorInfo* enum_info = lParam;

	if (enum_info->target_index == enum_info->current_index++) {
		/*
		* build the monitor info 
		* and stop enumerating
		*/

		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);

		BOOL res = GetMonitorInfo(
			hMonitor,
			&info);

		MonitorInfo* monitor_info = enum_info->out;

		monitor_info->absulutRect = *rect;
		
		monitor_info->relativeRect.right = rect->right - rect->left;
		monitor_info->relativeRect.bottom = rect->bottom - rect->top;
		monitor_info->relativeRect.left = 0;
		monitor_info->relativeRect.top = 0;

		monitor_info->index = enum_info->target_index;

		monitor_info->requierd_buffer_size = 
			_calculate_requierd_buffer_size(monitor_info);
		

		enum_info->res = true;		
		return FALSE;
	}

	return TRUE;
}

bool _get_info_by_index(int index, MonitorInfo* out) {
	EnumMonitorInfo info;
	ZeroMemory(&info, sizeof(info));

	info.out = out;
	info.target_index = index;

	EnumDisplayMonitors(
		GetDC(NULL),
		NULL, 
		_get_monitor_by_index_enum_proc,
		&info);

	return info.res;
}

int _calculate_requierd_buffer_size(MonitorInfo* info) {


	RECT target_rect;
	RECT current_rect = info->absulutRect;
	
	build_target_rect(
		g_ProgramConfig,
		&target_rect);

	bound_to_rect(
		&current_rect,
		&current_rect,
		target_rect,
		info,
		g_ProgramConfig);

	int buffer_size =
		current_rect.right *
		current_rect.bottom *
		g_ProgramConfig->target_bit_count;

	g_CurrentBufferSize = max(g_CurrentBufferSize, buffer_size);
	return g_CurrentBufferSize;
}

void _post_full_screen_capture_msg_timeout() {
	g_Waiting = true;
	UINT_PTR timer = SetTimer(
		g_MyWindow,
		CAPTURE_ALL_SCREEN_TIMER_EVENT,
		CAPTURE_FULL_SCREEN_TIMEOUT_ENTERVAL,
		NULL);
}

void _post_full_screen_capture_msg() {
	PostMessageW(
		g_MyWindow,
		WM_TIMER,
		CAPTURE_ALL_SCREEN_EVENT,
		NULL);
}

void CALLBACK _handle_win_event(
	HWINEVENTHOOK hook,
	DWORD event,
	HWND hwnd,
	LONG idObject,
	LONG idChild,
	DWORD dwEventThread,
	DWORD dwmsEventTime)
{

	switch (event)
	{
		case EVENT_OBJECT_FOCUS: {
			g_CurrentHWND = hwnd;
		
			RECT hWnd_rect;

			GetWindowRect(
				g_CurrentHWND, 
				&hWnd_rect);

			g_FocusedRect = hWnd_rect;
			_post_full_screen_capture_msg_timeout();

	}break;
	
	case EVENT_SYSTEM_MOVESIZESTART: {
		g_IsDragging = true;
		g_CurrentHWND = hwnd;

		RECT hWnd_rect;
		GetWindowRect(g_CurrentHWND, &hWnd_rect);

		DeltaStruct* last_delta = g_Buffers->back;
		memcpy(
			&last_delta->last_drag_pos, 
			&hWnd_rect, 
			sizeof(Rect));

		_post_full_screen_capture_msg_timeout();


	}break;

	case EVENT_SYSTEM_MOVESIZEEND: {
		g_IsDragging = false;

		_post_full_screen_capture_msg_timeout();


	}break;



	case EVENT_OBJECT_LOCATIONCHANGE: {
		if (g_IsDragging ||
			idObject != (LONG)NULL ||
			idChild != (LONG)NULL ||
			hwnd != g_CurrentHWND) {



			return;
		}

		_post_full_screen_capture_msg_timeout();




	}break;

	case EVENT_OBJECT_DESTROY: {
		if (event == EVENT_OBJECT_DESTROY &&
			hwnd == g_CurrentHWND &&
			idObject == OBJID_WINDOW &&
			idChild == INDEXID_CONTAINER) {

			_post_full_screen_capture_msg_timeout();
			}
		}break;
	}
}



bool _try_find_delta_in_window(
	RECT target_rect,
	DeltaStruct* last_delta,
	DeltaStruct* out) {

	bool can_union_to_upper_rect = false;

	int rect_width = target_rect.right - target_rect.left;
	int rect_height = (target_rect.bottom - target_rect.top) / g_ProgramConfig->num_of_delta_parts;

	int first_rect_extra_height =
		(target_rect.bottom - target_rect.top) % g_ProgramConfig->num_of_delta_parts;

	RECT current_rect;
	current_rect.left = 0;
	current_rect.right = rect_width;
	current_rect.top = 0;
	current_rect.bottom = rect_height + first_rect_extra_height;

	int i = 1;
	int current_buffer_rect = 0;

	bool delta_found = false;


	do {

		int offset =
			(current_rect.top) *
			(rect_width)*g_ProgramConfig->target_bit_count / 8;

		int size = (current_rect.bottom - current_rect.top) *
			(rect_width)*g_ProgramConfig->target_bit_count / 8;


		int res =
			memcmp(
				out->buffer + offset,
				last_delta->buffer + offset,
				size);

		if (res != 0) {
			{

				delta_found = true;

				if (can_union_to_upper_rect) {

					BufferRectDesk* last_desk =
						&out->rects[current_buffer_rect - 1];

					last_desk->buffer_size += size;

					UnionRect(
						&last_desk->rect,
						&current_rect,
						&last_desk->rect);

				}
				else {
					out->rects[current_buffer_rect].offset = offset;
					out->rects[current_buffer_rect].buffer_size = size;




					memcpy(
						&out->rects[current_buffer_rect].rect,
						&current_rect,
						sizeof(current_rect));

					current_buffer_rect++;
				}

				can_union_to_upper_rect = true;

			}
		}
		else {
			can_union_to_upper_rect = false;
		}


		current_rect.top = current_rect.bottom;
		current_rect.bottom += rect_height;




	} while (i++ < g_ProgramConfig->num_of_delta_parts);


	if (!delta_found) {
		return false;
	}


	out->num_of_rects = current_buffer_rect;

	for (int i = 0; i < out->num_of_rects; i++) {
		out->rects[i].rect.left += target_rect.left;
		out->rects[i].rect.right += target_rect.left;
		out->rects[i].rect.top += target_rect.top;
		out->rects[i].rect.bottom += target_rect.top;
	}


	return true;
}


void _handle_capture(
	RECT src_rect,
	DeltaStruct* front,
	DoubleBuffers* buffers,
	bool capture_all) {

	
	RECT full_window_target;
	build_target_rect(
		g_ProgramConfig, 
		&full_window_target);
	
	
	src_rect.right =  min(src_rect.right, g_CurrentMonitorInfo.absulutRect.right);
	src_rect.bottom = min(src_rect.bottom, g_CurrentMonitorInfo.absulutRect.bottom);
	src_rect.top = max(src_rect.top, g_CurrentMonitorInfo.absulutRect.top);
	src_rect.left = max(src_rect.left, g_CurrentMonitorInfo.absulutRect.left);

	bool res = IsRectEmpty(&src_rect);
	if (res) {
		return;
	}

	RECT target_rect;

	int capture_size;
	res = capture_screen(
		&g_CurrentMonitorInfo,
		&src_rect,
		&target_rect,
		front->buffer,
		&capture_size,
		g_ProgramConfig);


	if (!res) {
#ifdef _DEBUG
		platform_exit_with_error("Failed to capture screen\n");
#else
		_post_full_screen_capture_msg();
#endif
		
	}

	DeltaStruct* last_delta = buffers->back;


	memcpy(
		&front->full_rect,
		&target_rect,
		sizeof(target_rect));

	front->num_of_rects = 0;


	if (!capture_all && 
		memcmp(
			&front->full_rect,
			&last_delta->full_rect,
			sizeof(Rect)) == 0) {

		bool res =
			_try_find_delta_in_window(
				target_rect,
				last_delta,
				front);

		if (!res) {
			return;
		}
	}
	else {
		front->num_of_rects = 1;
		front->rects[0].buffer_size = capture_size;
		front->rects[0].offset = 0;
		memcpy(
			&front->rects[0].rect,
			&target_rect,
			sizeof(target_rect));
	}


	g_Procs->on_screen_changed(
		front,
		buffers);


}

void clear_thread_msg_queue() {
	MSG msg;
	while (PeekMessage(
		&msg, 
		NULL, 
		0, 
		0, 
		TRUE)) {
		continue;
	}
}

void _delta_finder_main_loop() {
	

	g_Buffers =
		safe_malloc(sizeof(DoubleBuffers));

	g_CurrentBufferSize = g_CurrentMonitorInfo.requierd_buffer_size;

	create_double_buffers_from_existing(
		create_delta_struct(
			g_CurrentBufferSize + sizeof(DeltaStruct),
			g_ProgramConfig->target_bit_count),
		create_delta_struct(
			g_CurrentBufferSize + sizeof(DeltaStruct),
			g_ProgramConfig->target_bit_count),
		g_Buffers);


	g_CurrentHWND = GetForegroundWindow();
	GetWindowRect(
		g_CurrentHWND, 
		&g_FocusedRect);
	
	g_Hook = SetWinEventHook(
		EVENT_MIN, 
		EVENT_MAX, 
		0, 
		_handle_win_event, 
		0, 
		0, 
		WINEVENT_OUTOFCONTEXT);

	if (g_Hook == NULL) {
		platform_exit_with_error("Failed to set win event hook\n");
		return;
	}
	
	WindowStruct* window_struct =
		safe_malloc(sizeof(WindowStruct));
	ZeroMemory(
		window_struct,
		sizeof(WindowStruct));

	window_struct->width = 0;
	window_struct->height = 0;
	window_struct->window_name = L"ScreenDeltaWindow";

	PlatformWindow window;
	bool res = create_window(
		window_struct,
		&window,
		false);

	if (!res) {
		platform_exit_with_error("Failed to create a window for screen delta\n");
		return;
	}

	g_MyWindow =
		get_window_hwnd(window);

	UINT_PTR timer = SetTimer(
		g_MyWindow,
		CAPTURE_ALL_SCREEN_EVENT,
		g_ProgramConfig->capture_full_screen_interval,
		NULL);


	if (timer == (UINT_PTR)NULL) {
		platform_exit_with_error("Failed to create a timer for screen delta\n");
		return;
	}

	RECT to_capture;

	ZeroMemory(
		&to_capture, 
		sizeof(to_capture));

	while (true) {
		bool capture_full_screen = false;

		lock_mutex(g_ChangeMonitorLock);

		g_CurrentMonitorInfo = g_NextMonitorInfo;
		if (g_changeCurrentMonitor) {

			int new_size = g_CurrentMonitorInfo.requierd_buffer_size;
			if (new_size > g_Buffers->front_buffer_size) {
				g_Buffers->front = safe_realoc(g_Buffers->front, new_size);
				g_Buffers->front_buffer_size = new_size;
			}

			clear_thread_msg_queue();
			_post_full_screen_capture_msg();
			
			g_changeCurrentMonitor = false;

			
			RECT hWnd_rect;
			GetWindowRect(
				g_CurrentHWND, 
				&hWnd_rect);

			bool res = collide(
				&g_CurrentMonitorInfo.absulutRect,
				&hWnd_rect);

			if (res) {
				GetWindowRect(
					g_CurrentHWND,
					&to_capture);
			}
		}
		
		release_mutex(g_ChangeMonitorLock);

		DeltaStruct* front = g_Buffers->front;
		
		MSG msg;
		ZeroMemory(&msg, sizeof(msg));

		if (PeekMessageA(&msg, NULL, 0, 0, TRUE)) {

			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}

		to_capture = g_FocusedRect;

		if (msg.message == WM_TIMER) {


			if (msg.wParam == CAPTURE_ALL_SCREEN_EVENT) {
				to_capture =
					g_CurrentMonitorInfo.absulutRect;
				g_Waiting = false;
				capture_full_screen = true;
			}
			else if (msg.wParam == CAPTURE_ALL_SCREEN_TIMER_EVENT) {
				to_capture =
					g_CurrentMonitorInfo.absulutRect;
				g_Waiting = false;
				capture_full_screen = true;

				KillTimer(
					g_MyWindow,
					CAPTURE_ALL_SCREEN_TIMER_EVENT);
			}

		}
		else {
			RECT hWnd_rect;
			GetWindowRect(
				g_CurrentHWND,
				&hWnd_rect);

			bool res =
				collide(
					&g_CurrentMonitorInfo.absulutRect,
					&hWnd_rect);
			if (!res) {
				continue;
			}

			to_capture = hWnd_rect;


			if (g_IsDragging) {
				DeltaStruct* last_delta = g_Buffers->back;
				DeltaStruct* current_delta = g_Buffers->front;

				res = UnionRect(
					&to_capture,
					&last_delta->last_drag_pos,
					&hWnd_rect);

				memcpy(
					&current_delta->last_drag_pos,
					&to_capture,
					sizeof(Rect));

				to_capture.right += DRAGED_RECT_SIDE_ADDED_EDGE;
				to_capture.bottom += DRAGED_RECT_SIDE_ADDED_EDGE;
				to_capture.top -= DRAGED_RECT_SIDE_ADDED_EDGE;
				to_capture.left -= DRAGED_RECT_SIDE_ADDED_EDGE;


			}
		}

		if (!capture_full_screen) {
			if (g_Waiting) {
				continue;
			}
		}

		_handle_capture(
			to_capture,
			front,
			g_Buffers,
			capture_full_screen);
		
	}



}

void init_delta_finder(
	ProgramConfig* program_config,
	ScreenDeltaProcs* procs) {
	/*
	* The app creates an invisible window for the message loop
	*/

	g_ProgramConfig = program_config;
	g_Procs = procs;

	delta_finder_set_current_monitor(0);
	g_CurrentMonitorInfo = g_NextMonitorInfo;

	bool res = 
		create_mutex(&g_ChangeMonitorLock);

	if (!res) {
		platform_exit_with_error("Failed to create mutex for the delta finder\n");
	}

	Thread thread;
	res =
		create_thread(_delta_finder_main_loop,
			NULL,
			&thread);

	if (!res) {
		platform_exit_with_error("Failed to create thread for the delta finder\n");
	}
}

void delta_finder_set_current_monitor(int index) {
	
	MonitorInfo info;
	ZeroMemory(&info, sizeof(info));

	bool res = _get_info_by_index(index, &info);
	if (!res) {
		return;
	}
	

	g_NextMonitorInfo = info;
	g_changeCurrentMonitor = true;
	
}

#endif

