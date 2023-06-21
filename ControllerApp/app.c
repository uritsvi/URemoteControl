#include <window.h>
#include <program_config.h>
#include <threads.h>
#include <input.h>
#include <double_buffers.h>
#include <delta_struct.h>
#include <error.h>
#include <control_channel.h>
#include <platfrom_memory_utils.h>

#include "send_io.h"
#include "receive_screen_buffer.h"

#pragma comment(lib, "ScreenDelta.lib")
#pragma comment(lib, "Platform.lib")
#pragma comment(lib, "Helpers.lib")

static PlatformWindow g_AppWindow;

static char* g_Uncompressed;
static int g_UncompressedSize;

static int g_CurrentMonitorIndex;

void _on_enter_exit_control_mode(bool b) {
	send_io_controll_key_callback(g_CurrentMonitorIndex);
}

void _reset_mouse_pos() {
	Rect window_rect;
	get_window_rect(
		g_AppWindow,
		&window_rect);

	int window_width = window_rect.right - window_rect.left;
	int window_height = window_rect.bottom - window_rect.top;
	set_cursor_pos(
		window_rect.left + (window_width / 2),
		window_rect.top + (window_height / 2));
}

void _handle_screen_buffer() {
	ProgramConfig* config = get_program_config();

	int full_buffer_size =
		sizeof(DeltaPacketInfo) + config->target_width * config->target_height * (config->target_bit_count / 8);

	Event work_done;
	bool res = create_event(&work_done);
	if (!res) {
		platform_exit_with_error("Failed to cerate work done event for sync screen buffer\n");
	}

	DoubleBuffers* buffers = safe_malloc(sizeof(DoubleBuffers));
	create_double_buffers(
		full_buffer_size,
		buffers);

	ZeroMemory(
		buffers->back,
		full_buffer_size);

	while (true) {
		

		receive_screen_buffer(
			buffers->front,
			work_done);

		DeltaPacket* current_delta = 
			buffers->back;

		int dest_len = g_UncompressedSize;
		res = uncompress(
			g_Uncompressed, 
			&dest_len, 
			buffers->back + sizeof(DeltaPacketInfo),
			current_delta->info.compressed_size);

		draw_to_window(
			g_AppWindow,
			config->target_bit_count,
			g_Uncompressed,
			current_delta->info.rect);

		wait_event(work_done);
		reset_event(work_done);
			


		switch_buffers(buffers);
		
	}
}

void _control_key_callback(char key) {
	g_CurrentMonitorIndex = atoi(&key);

	_reset_mouse_pos();
	send_io_controll_key_callback(g_CurrentMonitorIndex);
}

void _on_all_clients_connected() {
	_reset_mouse_pos();

	Thread thread;
	bool res = create_thread(
		_handle_screen_buffer, 
		NULL, 
		&thread);

	if (!res) {
		platform_exit_with_error("Failed to create thread to handle the screen buffer\n");
		return;
	}

	char controlKeys[UREMOTE_CONTROL_MAX_MONITORS];
	for (int i = 1; i < UREMOTE_CONTROL_MAX_MONITORS; i++) {
		controlKeys[i - 1] = i + '0';
	}

	g_CurrentMonitorIndex = 1;
	set_control_keys(
		controlKeys,
		UREMOTE_CONTROL_MAX_MONITORS,
		_control_key_callback);

	register_input_callback(send_io_input_callback);
}

void shut_down() {
	exit(0);
}

void run_app() {
	init_error(shut_down);

	ProgramConfig* config = get_program_config();

	g_UncompressedSize =
		config->target_width *
		config->target_height *
		(config->target_bit_count / 8);

	g_Uncompressed = 
		safe_malloc(g_UncompressedSize);

	WindowStruct* window_struct =
		safe_malloc(sizeof(WindowStruct));

	window_struct->width = config->window_width;
	window_struct->height = config->window_height;
	window_struct->window_name = APP_NAME;
	
	window_struct->shut_down_proc = shut_down;
	window_struct->on_enter_exit_control_mode = _on_enter_exit_control_mode;

	init_windows_system();
	bool res = create_window(
		window_struct,
		&g_AppWindow,
		true);


	if (!res) {
		platform_exit_with_error("Failed to create app window\n");
		return;
	}

	CallbacksTable table;
	ZeroMemory(table, sizeof(table) / sizeof(table[0]));

	table[ALL_CLIENTS_CONNECTED_MSG] = _on_all_clients_connected;

	handle_control_channel(
		config->server_address,
		config->server_port,
		config->client_index,
		table);

	
	show_window(g_AppWindow);


	init_receive_screen_buffer();
	
	init_input(g_AppWindow);
	init_send_io();


	while (true) {
		poll_events(g_AppWindow);
	}
}