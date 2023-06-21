#ifdef _WIN32

#include <Common.h>
#include <platfrom_memory_utils.h>
#include <platform.h>
#include <input.h>

#include <Windows.h>
#include <hidsdi.h>
#include <stdio.h>

#include <threads.h>

#include "input.h"

#pragma comment(lib, "platform.lib")


static InputCallback g_Callbacks[MAX_OBJECTS];
static int g_NextInputCallbackIndex;

static PlatformWindow g_Window;

static float g_DxExtra;
static float g_DyExtra;

static POINT g_LastFoucusMousePos;
static bool g_KiledFocuse;
static bool g_InControlMode;

static HHOOK g_KeyboardHook;
static HHOOK g_MouseHook;

static char* g_controlKeys;
static int g_numOfControlKeys;
static ControlKeysCallback g_controlKeysCallback;

void _handle_input_struct(InputStruct input) {
	for (int i = 0; i < g_NextInputCallbackIndex; i++) {
		g_Callbacks[i](input);
	}
}

void _handle_keyboard_input(
	UINT msg,
	KBDLLHOOKSTRUCT* info) {


	if (g_InControlMode && msg == WM_KEYDOWN) {
		for (int i = 0; i < g_numOfControlKeys; i++) {

			if (info->vkCode == g_controlKeys[i]) {

				g_controlKeysCallback(g_controlKeys[i]);
				break;
			}
		}
	}

	if (g_KiledFocuse) {
		return;
	}

	InputStruct input;
	ZeroMemory(&input, sizeof(input));

	input.type = KeyboadInputType;
	input.keyboardInput.keycode = info->vkCode;

	if (msg == WM_KEYDOWN ||
		msg == WM_SYSKEYDOWN) {

		input.keyboardInput.key_up = false;
	}
	else {
		input.keyboardInput.key_up = true;
	}

	_handle_input_struct(input);

}

void _handle_mouse_input(
	MSLLHOOKSTRUCT* info, 
	UINT msg) {

	InputStruct input;
	ZeroMemory(&input, sizeof(input));

	Rect window_rect;
	get_window_rect(g_Window, &window_rect);

	int width = window_rect.right - window_rect.left;
	int height = window_rect.bottom - window_rect.top;

	input.type = MouseInputType;
	input.mouseInput.dx = (float)(info->pt.x - window_rect.left) / width;
	input.mouseInput.dy = (float)(info->pt.y - window_rect.top) / height;


	if (msg == WM_MOUSEWHEEL) {
		input.mouseInput.flags |= MOUSE_EVENTF_WHEEL;
		input.mouseInput.wheel_delta = info->mouseData >> 16;
	}
	else if (msg == WM_LBUTTONDOWN) {
		input.mouseInput.flags |= MOUSE_EVENTF_LEFT_DOWN;
	}
	else if (msg == WM_LBUTTONUP) {
		input.mouseInput.flags |= MOUSE_EVENTF_LEFT_UP;
	}
	else if (msg == WM_RBUTTONDOWN) {
		input.mouseInput.flags |= MOUSEEVENTF_RIGHTDOWN;
	}
	else if (msg == WM_RBUTTONUP) {
		input.mouseInput.flags |= MOUSE_EVENTF_RIGHT_UP;
	}
	else if (msg == WM_MBUTTONDOWN) {
		input.mouseInput.flags |= MOUSE_EVENTF_MIDDLE_DOWN;
	}
	else if (msg == WM_MBUTTONUP) {
		input.mouseInput.flags |= MOUSE_EVENTF_MIDDLE_UP;
	}

	input.mouseInput.flags |= MOUSE_EVENTF_MOVE;


	_handle_input_struct(input);

}

LRESULT _keyboard_hook_proc(
	int nCode, 
	WPARAM wParam, 
	LPARAM lParam) {

	KBDLLHOOKSTRUCT* info = lParam;

	_handle_keyboard_input(
		wParam, 
		info);

	if (info->vkCode == VK_TAB && info->flags & LLKHF_ALTDOWN) {
		return 1;
	}

	if (info->vkCode == VK_LWIN) {
		return 1;
	}


	return CallNextHookEx(
		g_KeyboardHook, 
		nCode, 
		wParam, 
		lParam);
}

LRESULT _mouse_hook_proc(
	int nCode, 
	WPARAM wParam, 
	LPARAM lParam) {
	
	_handle_mouse_input(
		lParam, 
		wParam);

	return CallNextHookEx(
		g_MouseHook,
		nCode,
		wParam,
		lParam);

}

void _release_clip() {
	ClipCursor(NULL);
}

void _clip_input_to_window(HWND hWnd) {
	RECT rect;
	GetWindowRect(hWnd, &rect);
	
	ClipCursor(&rect);
}

uint32_t _convert_mouse_flags(uint32_t flags) {
	uint32_t out = 0;

	if (flags & RI_MOUSE_LEFT_BUTTON_DOWN) {
		out |= MOUSE_EVENTF_LEFT_DOWN;
	}
	if (flags & RI_MOUSE_LEFT_BUTTON_UP) {
		out |= MOUSE_EVENTF_LEFT_UP;
	}
	if (flags & RI_MOUSE_RIGHT_BUTTON_DOWN) {
		out |= MOUSE_EVENTF_RIGHT_DOWN;
	}
	if (flags & RI_MOUSE_RIGHT_BUTTON_UP) {
		out |= MOUSE_EVENTF_RIGHT_UP;
	}
	if (flags & RI_MOUSE_MIDDLE_BUTTON_DOWN) {
		out |= MOUSE_EVENTF_MIDDLE_DOWN;
	}
	if (flags & RI_MOUSE_MIDDLE_BUTTON_UP) {
		out |= MOUSE_EVENTF_MIDDLE_UP;
	}
	if (flags & RI_MOUSE_WHEEL) {
		out |= MOUSEEVENTF_WHEEL;
	}

	return out;
}

void shut_down_input() {
	for (int i = 0; i < 256; i++) {
		keybd_event(i, 0, KEYEVENTF_KEYUP, 0);
	}

	mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
	mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
	mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, 0);
}

void set_control_keys(
	char* keys,
	int num_of_keys,
	ControlKeysCallback callback) {

	g_numOfControlKeys = num_of_keys;
	g_controlKeysCallback = callback;
	
	g_controlKeys = (char*)safe_malloc(num_of_keys);
	
	memcpy_s(
		g_controlKeys, 
		num_of_keys, 
		keys, 
		num_of_keys);

}

bool init_input(
	PlatformWindow window) {

	_clip_input_to_window(get_window_hwnd(window));

	g_KeyboardHook = SetWindowsHookEx(
		WH_KEYBOARD_LL, 
		_keyboard_hook_proc, 
		NULL, 
		0);

	if (g_KeyboardHook == NULL) {
		return false;
	}
	g_MouseHook = SetWindowsHookEx(
		WH_MOUSE_LL,
		_mouse_hook_proc,
		NULL,
		0);

	if (g_MouseHook == NULL) {
		return false;
	}

	return true;
}

bool register_input_callback(InputCallback input_callback) {
	g_Callbacks[g_NextInputCallbackIndex++] = input_callback;

	return true;
}
void send_input(
	InputStruct input_struct,
	PlatformMonitorInfo monitor) {

	INPUT* input = safe_malloc(sizeof(INPUT));
	ZeroMemory(input, sizeof(INPUT));

	
	if (input_struct.type == KeyboadInputType) {
		input->ki.wVk = input_struct.keyboardInput.keycode;

		if (input_struct.keyboardInput.key_up) {
			input->ki.dwFlags |= KEYEVENTF_KEYUP;
		}

		input->type = INPUT_KEYBOARD;


	}
	else if(input_struct.type == MouseInputType) {
		if (input_struct.mouseInput.flags & MOUSE_EVENTF_MOVE) {



			int x = (input_struct.mouseInput.dx * monitor.width) + monitor.absulut_left;
			int y = (input_struct.mouseInput.dy * monitor.height) + monitor.absulut_top;

			SetCursorPos(x, y);

			input->mi.dwFlags = input->mi.dwFlags &~MOUSE_EVENTF_MOVE;

		}

		input->mi.mouseData = input_struct.mouseInput.wheel_delta;
		input->mi.dwFlags = input_struct.mouseInput.flags;

		input->type = INPUT_MOUSE;

	
	}
	

	SendInput(
		1,
		input,
		sizeof(INPUT));

	free(input);


	
}

void WMFocuse_callback(HWND hWnd) {
	if (g_KiledFocuse) {
		g_KiledFocuse = false;

		SetCursorPos(
			g_LastFoucusMousePos.x,
			g_LastFoucusMousePos.y);

		_clip_input_to_window(hWnd);
	}
}

void WMKillFocuse_callback(HWND hWnd) {

	_release_clip();

	GetCursorPos(&g_LastFoucusMousePos);
	g_KiledFocuse = true;
}

void set_cursor_pos(
	int x,
	int y) {

	SetCursorPos(x, y);
}

void EnterControlMode() {
	g_InControlMode = true;
}
void ExitControlMode() {
	g_InControlMode = false;
}


#endif