#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include <windows.h>

#include <zlib.h>

#include <Common.h>
#include <entry_point.h>
#include <window.h>
#include <platfrom_memory_utils.h>
#include <error.h>
#include <event_loop.h>
#include <screen_delta.h>
#include <input.h>
#include <console.h>
#include <stdio.h>
#include <threads.h>
#include <screen_delta.h>
#include <read_ini.h>

#pragma comment(lib, "Platform.lib")
#pragma comment(lib, "ScreenDelta.lib")
#pragma comment(lib "zlibwapi.lib")

#define BIT_COUNT 16

static bool g_IsRuning;
static PlatformWindow g_Window;

static int g_CurrentMonitorIndex;

void on_screen_changed(
	DeltaStruct* delta, 
	DoubleBuffers* buffers) {

	Rect rect;
	get_window_rect(g_Window, &rect);

	ProgramConfig* config = get_program_config();

	for (int i = 0; i < delta->num_of_rects; i++) {
		
		draw_to_window(
			g_Window,
			config->target_bit_count,
			delta->buffer + delta->rects[i].offset,
			delta->rects[i].rect,
			delta->rects[i].rect);


	}

	switch_buffers(buffers);
}

void shut_down() {
	shut_down_delta();
	g_IsRuning = false;
}

void _control_key_callback(char c) {
	g_CurrentMonitorIndex = atoi(&c) - 1;
	set_current_monirot(g_CurrentMonitorIndex);
}

void _on_enter_exit_control_mode(bool b) {
	set_current_monirot(g_CurrentMonitorIndex);
}

void entry_point(int argc,
				 char* argv) {

	init_error(NULL);

	ProgramConfig* config = safe_malloc(sizeof(ProgramConfig));
	read_ini(config);

	init_program_config(config);

	create_debug_console();

	init_windows_system();

	WindowStruct* window_struct = safe_malloc(sizeof(WindowStruct));

	window_struct->height = config->window_height;
	window_struct->width = config->window_width;
	window_struct->shut_down_proc = shut_down;
	window_struct->on_enter_exit_control_mode = _on_enter_exit_control_mode;
	window_struct->window_name = APP_NAME;

	bool res = create_window(window_struct,
							 &g_Window,
							 true);

	if (!res) {
		platform_exit_with_error("Failed to create window\n");
		return;
	}

	show_window(g_Window);

	ScreenDeltaProcs* procs =
		safe_malloc(sizeof(ScreenDeltaProcs));

	procs->on_screen_changed = on_screen_changed;

	init_screen_delta(config, procs);

	char controlKeys[10] = {0};
	for (int i = 1; i < UREMOTE_CONTROL_MAX_MONITORS; i++) {
		controlKeys[i - 1] = i + '0';
	}

	set_control_keys(controlKeys, 10, _control_key_callback);

	
	res = init_input(g_Window);
	if (!res) {
		platform_exit_with_error("Failed to init input");
		return -1;
	}

	g_IsRuning = true;
	while (g_IsRuning) {
		poll_events(g_Window);
	}

}
