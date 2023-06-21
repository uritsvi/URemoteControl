#ifndef __INPUT__
#define __INPUT__

#include <stdint.h>
#include <stdbool.h>
#include <window.h>

#include <monitors.h>

typedef enum {
	KeyboadInputType = 0,
	MouseInputType = 1,
	ControlInputType = 2,
} PlatformInputType;

typedef struct {

	/*
	* for now the scan codes of the program are identical to 
	* the windows vk codes, 
	linK:https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
	*/

	uint32_t keycode;
	bool key_up;

} PlatformKeyboadInput;

#define MOUSE_EVENTF_MOVE        0x0001
#define MOUSE_EVENTF_LEFT_DOWN    0x0002
#define MOUSE_EVENTF_LEFT_UP      0x0004
#define MOUSE_EVENTF_RIGHT_DOWN   0x0008
#define MOUSE_EVENTF_RIGHT_UP     0x0010
#define MOUSE_EVENTF_MIDDLE_DOWN  0x0020
#define MOUSE_EVENTF_MIDDLE_UP    0x0040 
#define MOUSE_EVENTF_X_DOWN       0x0080
#define MOUSE_EVENTF_X_UP         0x0100
#define MOUSE_EVENTF_WHEEL       0x0800


typedef struct {
	uint32_t flags;
	
	float dx;
	float dy;

	short wheel_delta;
}PlatformMouseInput;

typedef struct {
	int index;
} PlatformControlInputType;

typedef struct {
	PlatformInputType type;

	union Input
	{
		PlatformKeyboadInput keyboardInput;
		PlatformMouseInput mouseInput;
		PlatformControlInputType controlInput;
	};

} InputStruct;

typedef void(*InputCallback)(InputStruct input);
typedef void(*ControlKeysCallback)(char key);

bool init_input(
	PlatformWindow window);

/*
* when the program is in control 
* get a callback whene one of this keys are pressed
*/

void set_control_keys(
	char* keys,
	int num_of_keys,
	ControlKeysCallback callback);

void shut_down_input();

bool register_input_callback(InputCallback input_callback);

/*
* the monitor parameter is relevent just for the mouse, 
* if a keyboard input is sent the parameter can be NULL
*/

void send_input(
	InputStruct input, 
	PlatformWindow* monitor);

#ifdef _WIN32

#include <Windows.h>

void WMFocuse_callback(HWND hWnd);
void WMKillFocuse_callback(HWND hWnd);

#endif

void EnterControlMode();
void ExitControlMode();

void set_cursor_pos(
	int x, 
	int y);

#endif