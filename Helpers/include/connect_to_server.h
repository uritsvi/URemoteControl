#include <network.h>
#include <stdint.h>

#pragma comment(lib, "Platform.lib")

bool connect_to_server(const char* address,
					   const char* port,
					   uint32_t max_buffer_size,
					   uint32_t client_type,
				   	   char target,
					   char client_index,
					   bool notify_on_data_recived,
					   Socket* out);
