#include <stdbool.h>
#include <memory.h>

#include <Common.h>
#include <threads.h>
#include <error.h>

#include "control_channel.h"
#include "connect_to_server.h"

static CallbacksTable g_Callbacks_table;
static Socket g_Socket;

typedef void(*Callback)();

void _thread_proc() {
	while (true) {
		char msg;
		receive_one_byte(g_Socket, &msg);
		
		Callback callback = g_Callbacks_table[msg];
		if (callback == NULL) {
			platform_exit_with_error("Control chanel function callback is null\n");
			return;
		}
		callback();
	}
}

bool handle_control_channel(const char* address, 
							const char* port, 
							char client_index,
							CallbacksTable callbacks) {
	
	bool res = connect_to_server(
					  address, 
					  port, 
					  0, 
					  CONTROL_CHANEL_CLIENT_TYPE, 
					  CLIENT_TARGET_READ, 
					  client_index, 
					  false, 
					  &g_Socket);
	
	if (!res) {
		return res;
	}

	memcpy(&g_Callbacks_table,
		callbacks,
		sizeof(CallbacksTable));

	Thread thread;
	res = create_thread(_thread_proc,
						NULL, 
						&thread);
	if (!res) {
		return false;
	}

	return true;

}
