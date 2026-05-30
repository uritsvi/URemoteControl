#ifdef _WIN32

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <platfrom_memory_utils.h>
#include <Common.h>

#include "network.h"
#include "platform_threads.h"
#include "crypto.h"

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
	for (int i = 0; i < MAX_OBJECTS; i++) {
		if (g_Sockets[i] != NULL) {
			WIN32Socket* s = g_Sockets[i];
			if (s->socket != INVALID_SOCKET) {
				closesocket(s->socket);
				s->socket = INVALID_SOCKET;
			}
			free(s);
			g_Sockets[i] = NULL;
		}
	}
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


/*
 * End-to-end encrypted variants of send_data_with_size / receive_data.
 *
 * When no e2e key is configured these behave exactly like the plaintext
 * length-prefixed helpers, so the wire format is unchanged. When a key is set
 * the payload is sealed with AES-256-GCM before the length prefix, which means
 * the relay server only ever forwards opaque ciphertext.
 */
bool send_encrypted_data(
	Socket socket,
	char* buffer,
	int size) {

	if (!crypto_is_enabled()) {
		return send_data_with_size(socket, buffer, size);
	}

	int cap = size + CRYPTO_OVERHEAD;
	unsigned char* sealed = (unsigned char*)malloc(cap);
	if (sealed == NULL) {
		return false;
	}

	int sealed_len = crypto_seal(
		(const unsigned char*)buffer,
		size,
		sealed,
		cap);

	if (sealed_len < 0) {
		free(sealed);
		return false;
	}

	bool res = send_uint32(socket, (uint32_t)sealed_len);
	res &= send_data(socket, (char*)sealed, sealed_len);

	free(sealed);
	return res;
}

bool receive_encrypted_data(
	Socket socket,
	char* buffer,
	int* size) {

	if (!crypto_is_enabled()) {
		return receive_data(socket, buffer, size);
	}

	WIN32Socket* cur_socket = g_Sockets[socket];

	uint32_t sealed_len = 0;
	bool res = _socket_receive(
		cur_socket,
		(char*)&sealed_len,
		sizeof(sealed_len));

	if (!res || sealed_len < CRYPTO_OVERHEAD) {
		return false;
	}

	unsigned char* sealed = (unsigned char*)malloc(sealed_len);
	if (sealed == NULL) {
		return false;
	}

	res = _socket_receive(
		cur_socket,
		(char*)sealed,
		(int)sealed_len);

	if (!res) {
		free(sealed);
		return false;
	}

	int pt_len = crypto_open(
		sealed,
		(int)sealed_len,
		(unsigned char*)buffer,
		0x7fffffff);

	free(sealed);

	if (pt_len < 0) {
		return false;
	}

	*size = pt_len;
	return true;
}

/*
 * Plaintext length-prefixed key blob helpers for the public-key handshake.
 * These deliberately bypass the encryption layer: they run before a session key
 * exists and only carry public keys, which are not secret.
 *   wire: [ uint32 length ][ length bytes ]
 */
static bool _send_key_blob(
	Socket socket,
	const unsigned char* buffer,
	int size) {

	bool res = send_uint32(socket, (uint32_t)size);
	if (size > 0) {
		res &= send_data(socket, (char*)buffer, size);
	}
	return res;
}

static bool _receive_key_blob(
	Socket socket,
	unsigned char* buffer,
	int cap,
	int* size) {

	WIN32Socket* cur_socket = g_Sockets[socket];

	uint32_t len = 0;
	bool res = _socket_receive(
		cur_socket,
		(char*)&len,
		sizeof(len));

	if (!res || len == 0 || (int)len > cap) {
		return false;
	}

	res = _socket_receive(
		cur_socket,
		(char*)buffer,
		(int)len);

	if (!res) {
		return false;
	}

	*size = (int)len;
	return true;
}

bool network_send_public_key(Socket socket) {
	unsigned char my_pub[CRYPTO_PUBLIC_KEY_SIZE];
	int my_len = 0;

	if (crypto_is_configured()) {
		my_len = crypto_export_public_key(my_pub, (int)sizeof(my_pub));
		if (my_len < 0) {
			return false;
		}
	}

	/* A zero-length blob keeps the handshake layout uniform when E2E is off. */
	return _send_key_blob(socket, my_pub, my_len);
}

bool network_receive_peer_key(Socket socket) {
	if (!crypto_is_configured()) {
		/* E2E off: the relay sends no peer key, so there is nothing to read. */
		return true;
	}

	unsigned char peer_pub[CRYPTO_PUBLIC_KEY_SIZE];
	int peer_len = 0;
	if (!_receive_key_blob(socket, peer_pub, (int)sizeof(peer_pub), &peer_len)) {
		return false;
	}

	return crypto_derive_session_key(peer_pub, peer_len);
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
		closesocket(win32_socket);
		return false;
	}

	res = connect(win32_socket, 
				  address_info->ai_addr, 
				  (int)address_info->ai_addrlen);

	freeaddrinfo(address_info);

	if (res == SOCKET_ERROR) {
		closesocket(win32_socket);
		return false;
	}

	WIN32Socket* _socket = 
		(WIN32Socket*)safe_malloc(sizeof(WIN32Socket));

	_socket->socket = win32_socket;
	_socket->closed = false;
	_socket->out = g_NextSendSocketIndex;

	*out = g_NextSendSocketIndex++;
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

		if (res == 0) {
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
