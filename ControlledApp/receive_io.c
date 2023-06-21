#include <Common.h>
#include <connect_to_server.h>
#include <network.h>
#include <threads.h>
#include <program_config.h>
#include <error.h>

#include "receive_io.h"

static Socket g_Socket;
static Event g_WorkToDo;

void init_receive_io() {
	ProgramConfig* config = get_program_config();

	bool res = connect_to_server(
		config->server_address, 
		config->server_port, 
		0, 
		SYNC_IO_CLIENT_TYPE, 
		CLIENT_TARGET_READ, 
		CONTROLED_CLIENT_INDEX, 
		false, 
		&g_Socket);

	if (!res) {
		platform_exit_with_error("Faield to connect sync io tunnel\n");
		return;
	}
}

void receive_io(InputStruct* buffer) {

	int size;

	bool res =
		receive_data(
			g_Socket,
			buffer,
			&size);

	if (!res) {
		platform_exit_with_error("Failed to receive io\n");
	}
}
