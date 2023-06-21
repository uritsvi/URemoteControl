#include <network.h>
#include <threads.h>
#include <input.h>
#include <Common.h>
#include <program_config.h>
#include <error.h>

static Socket g_Socket;
static Event g_WorkToDo;

static InputStruct g_InputQue[DEFAULT_BUFFER_SIZE];

static int g_NextInputCallbackIndex;

static Mutex g_Lock;

void _send_io() {
	InputStruct input_que[DEFAULT_BUFFER_SIZE];
	ZeroMemory(input_que, sizeof(input_que));

	int last_input_index = 0;

	while (true) {
		lock_mutex(g_Lock);

		for (int i = 0; i < g_NextInputCallbackIndex; i++) {
		

			input_que[i] = g_InputQue[i];
		}
		last_input_index = g_NextInputCallbackIndex;
		g_NextInputCallbackIndex = 0;

		release_mutex(g_Lock);

		for (int i = 0; i < last_input_index; i++) {
			
			bool res = send_data_with_size(
				g_Socket, 
				&input_que[i],
				sizeof(InputStruct));
				
			if (!res) {
				platform_exit_with_error("Faield to send input\n");
				return;
			}

			
		}
	}
}

void send_io_input_callback(InputStruct input) {
	lock_mutex(g_Lock);

	g_InputQue[g_NextInputCallbackIndex++] = input;

	release_mutex(g_Lock);
}

void send_io_controll_key_callback(int index) {
	InputStruct input;
	ZeroMemory(&input, sizeof(input));

	input.type = ControlInputType;
	input.controlInput.index = index;

	lock_mutex(g_Lock);

	g_InputQue[g_NextInputCallbackIndex++] = input;

	release_mutex(g_Lock);
}

void init_send_io() {
	ProgramConfig* config = get_program_config();

	
	bool res = connect_to_server(
		config->server_address,
		config->server_port,
		config->max_send_buffer,
		SYNC_IO_CLIENT_TYPE,
		CLIENT_TARGET_WRITE,
		CONTROLER_CLIENT_INDEX,
		false,
		&g_Socket);

	if (!res) {
		platform_exit_with_error("Failed to connect sync io tunnel\n");
		return;
	}

	
	Thread thread;
	res = create_thread(
		_send_io, 
		NULL, 
		&thread);
		
	if (!res) {
		platform_exit_with_error("Failed to create thread for sending io\n");
		return;
	}

	res = create_mutex(&g_Lock);
	if (!res) {
		platform_exit_with_error("Failed to create a mutex for sending io\n");
		return;
	}
}
