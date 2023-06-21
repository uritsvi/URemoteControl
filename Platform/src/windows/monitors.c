#include <monitors.h>

typedef struct {
	int target_index;
	int current_index;

	PlatformMonitorInfo* out;
	bool res;
} EnumMonitorInfo;

bool monitor_from_window(
	PlatformWindow window,
	PlatformMonitorInfo* out) {

	HWND hWnd = get_window_hwnd(window);

	HMONITOR hMonitor = 
		MonitorFromWindow(
			hWnd, 
			MONITOR_DEFAULTTONEAREST);

	if (hMonitor == NULL) {
		return false;
	}

	MONITORINFO info;
	GetMonitorInfo(
		hMonitor, 
		&info);

	out->width = info.rcMonitor.right - info.rcMonitor.left;
	out->height = info.rcMonitor.bottom - info.rcMonitor.top;
	
	out->absulut_left = info.rcMonitor.left;
	out->absulut_top = info.rcMonitor.top;

	return true;
}

BOOL _platfrom_get_monitor_by_index_enum_proc(
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

		enum_info->out->absulut_left = info.rcMonitor.left;
		enum_info->out->absulut_top = info.rcMonitor.top;
		
		enum_info->out->width = info.rcMonitor.right - info.rcMonitor.left;
		enum_info->out->height = info.rcMonitor.bottom - info.rcMonitor.top;
		enum_info->res = true;

		return FALSE;
	}

	return TRUE;
}


bool monitor_from_index(
	int index,
	PlatformMonitorInfo* out
) 
{
	EnumMonitorInfo info;
	ZeroMemory(&info, sizeof(info));

	info.out = out;
	info.target_index = index;
	info.out = out;

	EnumDisplayMonitors(
		GetDC(NULL),
		NULL,
		_platfrom_get_monitor_by_index_enum_proc,
		&info);

	return info.res;
}