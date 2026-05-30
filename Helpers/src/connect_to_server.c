#include "connect_to_server.h"

#pragma comment(lib, "Platform.lib")

/*
* the max_buffer_size is relevent just for a send socket,
* the value of this parameter in a receive socket does not 
* effect the beheivior of the server
*/

bool connect_to_server(const char* address,
					   const char* port,
					   uint32_t max_buffer_size,
					   uint32_t client_type,
					   char target,
					   char client_index,
					   bool notify_on_data_recived,
					   Socket* out) {

	Socket socket;
	bool res = create_socket(address, 
						     port, 
							 &socket);

	if (!res) {
		return res;
	}

	res = send_uint32(socket,
					  max_buffer_size);
	if (!res) {
		return res;
	}

	res = send_uint32(socket, 
					  client_type);
	if (!res) {
		return res;
	}
	
	res = send_one_byte(socket, 
						target);
	if (!res) {
		return res;
	}
	
	res = send_one_byte(socket, 
						client_index);
	if (!res) {
		return res;
	}

	res = send_one_byte(socket,
						notify_on_data_recived);
	if (!res) {
		return res;
	}

	/*
	 * Public key for the end-to-end key exchange, sent as the last field of the
	 * connection init data (a zero-length blob when E2E is off). The relay reads
	 * it and later hands each client its peer's key over the control channel.
	 */
	res = network_send_public_key(socket);
	if (!res) {
		return res;
	}

	bool client_connected;
	receive_one_byte(socket, 
					 &client_connected);

	if (!client_connected) {
		return false;
	}

	*out = socket;
	return true;

}