#ifdef _WIN32

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdbool.h>

#include <platfrom_memory_utils.h>
#include <Common.h>

#include "network.h"
#include "threads.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")

typedef struct {
	SOCKET socket;
	bool closed;
	Socket out;
} WIN32Socket;

static WIN32Socket* g_Sockets[MAX_OBJECTS] = { 0 };

static int g_NextSendSocketIndex;
static int g_NextReceiveSocketIndex;

static int g_MaxSendBufferSize;

bool _socket_send(WIN32Socket* socket, char* buffer, int size);
bool _socket_receive(WIN32Socket* socket, char* buffer, int size);

bool init_networking(int max_send_buffer_size) {

	WSADATA wsa_data;
	int res = WSAStartup(2, &wsa_data);
	if (res != 0) {
		return false;
	}

	g_MaxSendBufferSize = max_send_buffer_size;

	return true;

}

bool clean_up_networking() {
	int res = WSACleanup();
	if (res != 0) {
		return false;
	}
	return true;
}

bool send_uint32(Socket socket, 
				uint32_t buffer) {
	int cur_id = socket;

	WIN32Socket* cur_socket = g_Sockets[cur_id];


	bool res = _socket_send(cur_socket,
		(char*)&buffer,
		sizeof(buffer));

	return res;
}

bool send_one_byte(Socket socket,
				   char buffer) {
	int cur_id = socket;

	WIN32Socket* cur_socket = g_Sockets[cur_id];

	bool res = _socket_send(cur_socket, 
						    &buffer, 
						    sizeof(buffer));

	return res;
}

bool send_data(
	Socket socket,
	char* buffer,
	int size) {

	WIN32Socket* cur_socket =
		g_Sockets[socket];

	int i = 0;
	bool res = true;
	
	while (i < size - g_MaxSendBufferSize) {
			res &= _socket_send(
			cur_socket,
			buffer + i,
			g_MaxSendBufferSize);

		i += g_MaxSendBufferSize;
	}
	if (i < size) {
		res &= _socket_send(
			cur_socket,
			buffer + i,
			size - i);

	}

	return res;
}

bool send_data_with_size(
			   Socket socket, 
			   char* buffer, 
			   int size) {
	
	int cur_id = socket;

	WIN32Socket* cur_socket = g_Sockets[cur_id];

	bool res = send_uint32(
		socket, 
		size);

	res &= send_data(
		socket, 
		buffer, 
		size);
	
	return res;

}

bool receive_one_byte(Socket socket,
					  char* buffer) {
	int cur_id = socket;

	WIN32Socket* cur_socket = g_Sockets[cur_id];

	bool res = _socket_receive(cur_socket, 
							   buffer, 
							   sizeof(*buffer));

	return res;
}

bool receive_data(Socket socket, 
				  char* buffer, 
				  int* size) {

	int cur_id = socket;

	WIN32Socket* cur_socket = g_Sockets[cur_id];



	bool res = _socket_receive(cur_socket,
		size,
		sizeof(*size));



 	res &= _socket_receive(cur_socket, 
							buffer, 
							*size);

	return res;
}


bool create_socket(const char* address, 
				   const char* port, 
				   Socket* out) {

	SOCKET win32_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (win32_socket == INVALID_SOCKET) {
		return false;
	}

	struct addrinfo hints;
	struct addrinfo* address_info;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	int res = getaddrinfo(address, 
						  port, 
						  &hints, 
						  &address_info);

	if (res == SOCKET_ERROR) {
		return false;
	}

	res = connect(win32_socket, 
				  address_info->ai_addr, 
				  (int)address_info->ai_addrlen);

	if (res == SOCKET_ERROR) {
		return false;
	}

	*out = g_NextSendSocketIndex++;

	WIN32Socket* _socket = 
		(WIN32Socket*)safe_malloc(sizeof(WIN32Socket));

	_socket->socket = win32_socket;
	_socket->closed = false;
	_socket->out = *out;

	g_Sockets[*out] = _socket;
	
	return true;

}

bool _socket_send(WIN32Socket* socket, 
				  char* buffer, 
				  int size) {
	
	int res = send(socket->socket,
		buffer,
		size,
		0);

	if (res == SOCKET_ERROR) {
		return false;
	}
	return true;
}

bool _socket_receive(WIN32Socket* socket, 
					 char* buffer, 
				     int size) {
	
	int i = 0;
	while (i < size) {
		int res = recv(socket->socket,
			buffer + i,
			size - i,
			0);

		if (res == SOCKET_ERROR) {
			return false;
		}

		i += res;
	}
	
	return true;

}

bool receive_uint32(Socket socket, 
				   uint32_t* buffer) {
	
	int cur_id = socket;

	WIN32Socket* cur_socket = g_Sockets[cur_id];
	
	bool res = _socket_receive(cur_socket, 
							  (char*)buffer, sizeof(int));

	return res;
}

#endif