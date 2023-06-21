#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include <screen_delta.h>
#include <Common.h>
#include <platfrom_memory_utils.h>
#include <window.h>
#include <error.h>
#include <event_loop.h>
#include <program_config.h>
#include <zero_memory.h>
#include <input.h>
#include <platform.h>
#include <console.h>
#include <app_instances.h>
#include <monitors.h>
#include <control_channel.h>

#include "app.h"
#include "send_screen_buffer.h"
#include "receive_io.h"


#pragma comment(lib, "ScreenDelta.lib")
#pragma comment(lib, "Platform.lib")

static PlatformWindow g_AppWindow;
static PlatformMonitorInfo g_MonitorInfo;


void _handle_input() {
	InputStruct input;

	while (true) {
		receive_io(&input);
		if (input.type == ControlInputType) {
			int monitor_index = input.controlInput.index - 1;

			bool res = 
				monitor_from_index(
					monitor_index, 
					&g_MonitorInfo);

			if (!res) {
				// the monitor index is invalid 
				continue;
			}

			set_current_monirot(monitor_index);

			continue;
		}

		
		send_input(input, &g_MonitorInfo);

	}
}

void _on_all_clients_connected() {
	Rect screen_rect;
	ZeroMemory(&screen_rect, sizeof(screen_rect));

	PlatformMonitorInfo info;
	bool res = monitor_from_window(
		g_AppWindow,
		&info);
	if (!res) {
		platform_exit_with_error("Failed to get monitor info\n");
		return;
	}

	screen_rect.bottom = info.height;
	screen_rect.right = info.width;

	ProgramConfig* config = get_program_config();

	set_cursor_pos(
		(screen_rect.right / 2), 
		(screen_rect.bottom / 2));

	ScreenDeltaProcs* procs = 
		safe_malloc(sizeof(ScreenDeltaProcs));
	
	procs->on_screen_changed = 
		on_new_delta;

	init_screen_delta(
		config,
		procs);

	res = monitor_from_index(
			0, 
			&g_MonitorInfo);

	if (!res) {
		platform_exit_with_error("Failed to get monitor info\n");
	}

	Thread thread;
	res = create_thread(
		_handle_input,
		NULL,
		&thread);
		
	if (!res) {
		platform_exit_with_error("Failed to create thread for handling input\n");
		return;
	}
	
}

void shut_down() {
	shut_down_input();
	exit(0);


}


void run_app() {

	init_error(shut_down);
	create_debug_console();

	ProgramConfig* config = get_program_config();

	init_send_screen_buffer();
	init_receive_io();

	WindowStruct* window_struct =
		safe_malloc(sizeof(WindowStruct));

	window_struct->width = 0;
	window_struct->height = 0;
	window_struct->window_name = APP_NAME;
	
	window_struct->shut_down_proc = shut_down;

	init_windows_system();
	bool res = create_window(
		window_struct,
		&g_AppWindow,
		false);

	if (!res) {
		platform_exit_with_error("Failed to create app window\n");
		return;
	}

	CallbacksTable table;
	ZeroMemory(table, sizeof(table) / sizeof(table[0]));

	table[ALL_CLIENTS_CONNECTED_MSG] = _on_all_clients_connected;
	table[DATA_RECIVED_MSG] = on_screen_delta_recived;

	handle_control_channel(
		config->server_address,
		config->server_port,
		config->client_index,
		table);


	while (true) {
		poll_events(g_AppWindow);
	}
}