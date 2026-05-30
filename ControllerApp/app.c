#include <window.h>
#include <program_config.h>
#include <platform_threads.h>
#include <input.h>
#include <double_buffers.h>
#include <delta_struct.h>
#include <error.h>
#include <control_channel.h>
#include <platfrom_memory_utils.h>
#include <crypto.h>

#include <zlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <share.h>

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

static FILE* g_DbgLog = NULL;
static void _dbg(const char* fmt, ...) {
	if (g_DbgLog == NULL) {
		return;
	}
	va_list a;
	va_start(a, fmt);
	vfprintf(g_DbgLog, fmt, a);
	va_end(a);
	fflush(g_DbgLog);
}

void _handle_screen_buffer() {
	ProgramConfig* config = get_program_config();

	g_DbgLog = _fsopen("controller_dbg.log", "w", _SH_DENYWR);

	int raw_frame_size =
		config->target_width * config->target_height * (config->target_bit_count / 8);

	/*
	 * The received payload is [DeltaPacketInfo][zlib data]; the zlib data can be
	 * slightly larger than the raw frame for incompressible screens, so size the
	 * receive buffer with compressBound to match the sender and avoid an
	 * overflow.
	 */
	int full_buffer_size =
		sizeof(DeltaPacketInfo) + compressBound(raw_frame_size);

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

		Rect _r = current_delta->info.rect;
		int _w = _r.right - _r.left;
		int _h = _r.bottom - _r.top;
		_dbg("frame comp=%d rect=(%d,%d,%d,%d) w=%d h=%d draw_bytes=%d uncbuf=%d\n",
			current_delta->info.compressed_size, _r.left, _r.top, _r.right, _r.bottom,
			_w, _h, _w * _h * (config->target_bit_count / 8), g_UncompressedSize);

		int dest_len = g_UncompressedSize;
		res = uncompress(
			g_Uncompressed,
			&dest_len,
			buffers->back + sizeof(DeltaPacketInfo),
			current_delta->info.compressed_size);

		_dbg("  uncompress res=%d dest_len=%d\n", res, dest_len);

		draw_to_window(
			g_AppWindow,
			config->target_bit_count,
			g_Uncompressed,
			current_delta->info.rect);

		_dbg("  drawn\n");

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
	ProgramConfig* config = get_program_config();

	/*
	 * By the time this fires the control channel has already completed the
	 * end-to-end key exchange (peer public key delivered by the relay), so the
	 * session key is ready before any screen/input data flows.
	 */
	if (!config->debug_mode) {
		_reset_mouse_pos();
	}

	Thread thread;
	bool res = create_thread(
		_handle_screen_buffer,
		NULL,
		&thread);

	if (!res) {
		platform_exit_with_error("Failed to create thread to handle the screen buffer\n");
		return;
	}

	if (config->debug_mode) {
		/*
		 * Debug mode: do not bind the monitor-switch control keys and do not
		 * register the input-forwarding callback, so the local mouse and
		 * keyboard stay usable. The remote screen is still displayed.
		 */
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

	/* Prove end-to-end encryption is active: status, key id and self-test go to
	 * the console and to bin\client_e2e_<pid>.log once the handshake completes. */
	crypto_log_enable();

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

	
	set_window_start_windowed(config->debug_mode);
	show_window(g_AppWindow);


	init_receive_screen_buffer();

	if (!config->debug_mode) {
		/*
		 * Debug mode keeps the global keyboard/mouse hooks and the cursor clip
		 * disabled so the local machine stays usable while testing.
		 */
		init_input(g_AppWindow);
	}
	init_send_io();


	while (true) {
		poll_events(g_AppWindow);
	}
}