#ifndef __CONTROL__CHANNEL__
#define __CONTROL__CHANNEL__

#include <Common.h>

#include "connect_to_server.h"

typedef void* CallbacksTable[BITE_MAX_VALUE];

void create_callbacks_table(CallbacksTable* out);

bool handle_control_channel(const char* address,
	const char* port,
	char client_index,
	CallbacksTable callbacks);

#endif