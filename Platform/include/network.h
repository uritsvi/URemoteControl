#ifndef __NETWORK__
#define __NETWORD__

#include <stdint.h>

#include <stdbool.h>

typedef int Socket;

bool init_networking(int max_send_buffer_size);

bool clean_up_networking();

bool create_socket(
	const char* address, 
	const char* port, 
	Socket* out);

bool send_one_byte(
	Socket socket, 
	char buffer);

bool send_uint32(
	Socket socket, 
	uint32_t buffer);

bool send_data(
	Socket socket,
	char* buffer,
	int size);

bool send_data_with_size(
	Socket socket,
	char* buffer,
	int size);

bool receive_uint32(
	Socket socket, 
	uint32_t* buffer);

bool receive_one_byte(
	Socket socket,
	char* buffer);

bool receive_data(
	Socket socket, 
	char* buffer, 
	int* size);

#endif