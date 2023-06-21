#include <threads.h>
#include <network.h>
#include <program_config.h>
#include <Common.h>
#include <error.h>

#include "receive_screen_buffer.h"

static Event g_WorkToDo;
static Event g_WorkDone;
static Socket g_Socket;

static char* g_CurrentBuffer;

void _receive_screen_buffer_proc() {
	while (true) {
		wait_event(g_WorkToDo);

		int size;

		bool res =
			receive_data(
				g_Socket,
				g_CurrentBuffer,
				&size);

		if (!res) {
			platform_exit_with_error("Failed to receive data\n");
		}

		reset_event(g_WorkToDo);
		set_event(g_WorkDone);
	}
}

void init_receive_screen_buffer() {
	ProgramConfig* config =
		get_program_config();


	bool res =
		connect_to_server(
			config->server_address,
			config->server_port,
			0,
			SYNC_SCREEN_BUFFER_CLIENT_TYPE,
			CLIENT_TARGET_WRITE,
			config->client_index,
			false,
			&g_Socket);

	if (!res) {
		platform_exit_with_error("Failed to connect to server\n");
		return;
	}

	res = create_event(&g_WorkToDo);
	if (!res) {
		platform_exit_with_error("Failed to create event for receive screen buffer\n");
		return;
	}

	Thread thread;
	res = create_thread(
		_receive_screen_buffer_proc,
		NULL,
		&thread);

	if (!res) {
		platform_exit_with_error("Failed to create thread for receive screen buffer\n");
		return;
	}
}
void receive_screen_buffer(
	char* out,
	Event work_done) {


	g_CurrentBuffer = out;
	g_WorkDone = work_done;
	set_event(g_WorkToDo);

}