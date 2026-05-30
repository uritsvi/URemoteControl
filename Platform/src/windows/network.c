#ifdef _WIN32

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <platfrom_memory_utils.h>
#include <Common.h>

#include "network.h"
#include "platform_threads.h"
#include "crypto.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "libssl.lib")
#pragma comment (lib, "libcrypto.lib")

typedef struct {
	SOCKET socket;
	bool closed;
	Socket out;
	SSL* ssl;
	bool use_tls;
} WIN32Socket;

static WIN32Socket* g_Sockets[MAX_OBJECTS] = { 0 };

static int g_NextSendSocketIndex;
static int g_NextReceiveSocketIndex;

static int g_MaxSendBufferSize;

static bool g_UseTls = false;
static char g_TlsServerName[DEFAULT_BUFFER_SIZE] = { 0 };
static char g_TlsCaCertPath[DEFAULT_BUFFER_SIZE] = { 0 };

static SSL_CTX* g_SslCtx = NULL;

bool _socket_send(WIN32Socket* socket, char* buffer, int size);
bool _socket_receive(WIN32Socket* socket, char* buffer, int size);

void network_set_tls_config(bool use_tls, const char* server_name, const char* ca_cert_path) {
	g_UseTls = use_tls;
	if (server_name != NULL) {
		strncpy_s(g_TlsServerName, DEFAULT_BUFFER_SIZE, server_name, _TRUNCATE);
	} else {
		g_TlsServerName[0] = '\0';
	}
	if (ca_cert_path != NULL) {
		strncpy_s(g_TlsCaCertPath, DEFAULT_BUFFER_SIZE, ca_cert_path, _TRUNCATE);
	} else {
		g_TlsCaCertPath[0] = '\0';
	}
}

static bool _init_ssl_ctx(void) {
	if (g_SslCtx != NULL) {
		return true;
	}

	SSL_library_init();
	SSL_load_error_strings();
	OpenSSL_add_all_algorithms();

	g_SslCtx = SSL_CTX_new(TLS_client_method());
	if (g_SslCtx == NULL) {
		ERR_print_errors_fp(stderr);
		return false;
	}

	SSL_CTX_set_min_proto_version(g_SslCtx, TLS1_2_VERSION);

	if (g_TlsCaCertPath[0] != '\0') {
		if (SSL_CTX_load_verify_locations(g_SslCtx, g_TlsCaCertPath, NULL) != 1) {
			ERR_print_errors_fp(stderr);
			SSL_CTX_free(g_SslCtx);
			g_SslCtx = NULL;
			return false;
		}
	} else {
		if (SSL_CTX_set_default_verify_paths(g_SslCtx) != 1) {
			ERR_print_errors_fp(stderr);
			SSL_CTX_free(g_SslCtx);
			g_SslCtx = NULL;
			return false;
		}
	}

	return true;
}

static void _cleanup_ssl_ctx(void) {
	if (g_SslCtx != NULL) {
		SSL_CTX_free(g_SslCtx);
		g_SslCtx = NULL;
	}
	EVP_cleanup();
}

bool init_networking(int max_send_buffer_size) {

	WSADATA wsa_data;
	int res = WSAStartup(2, &wsa_data);
	if (res != 0) {
		return false;
	}

	g_MaxSendBufferSize = max_send_buffer_size;

	if (g_UseTls) {
		if (!_init_ssl_ctx()) {
			WSACleanup();
			return false;
		}
	}

	return true;

}

bool clean_up_networking() {
	for (int i = 0; i < MAX_OBJECTS; i++) {
		if (g_Sockets[i] != NULL) {
			WIN32Socket* s = g_Sockets[i];
			if (s->use_tls && s->ssl != NULL) {
				SSL_shutdown(s->ssl);
				SSL_free(s->ssl);
				s->ssl = NULL;
			}
			if (s->socket != INVALID_SOCKET) {
				closesocket(s->socket);
				s->socket = INVALID_SOCKET;
			}
			free(s);
			g_Sockets[i] = NULL;
		}
	}
	_cleanup_ssl_ctx();
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
	_socket->ssl = NULL;
	_socket->use_tls = false;

	if (g_UseTls && g_SslCtx != NULL) {
		SSL* ssl = SSL_new(g_SslCtx);
		if (ssl == NULL) {
			free(_socket);
			closesocket(win32_socket);
			return false;
		}

		SSL_set_fd(ssl, (int)win32_socket);

		const char* sni_name = (g_TlsServerName[0] != '\0') ? g_TlsServerName : address;
		SSL_set_tlsext_host_name(ssl, sni_name);

		if (SSL_connect(ssl) != 1) {
			ERR_print_errors_fp(stderr);
			SSL_free(ssl);
			free(_socket);
			closesocket(win32_socket);
			return false;
		}

		if (SSL_get_verify_result(ssl) != X509_V_OK) {
			SSL_shutdown(ssl);
			SSL_free(ssl);
			free(_socket);
			closesocket(win32_socket);
			return false;
		}

		_socket->ssl = ssl;
		_socket->use_tls = true;
	}

	*out = g_NextSendSocketIndex++;
	g_Sockets[*out] = _socket;
	
	return true;

}

bool _socket_send(WIN32Socket* socket, 
				  char* buffer, 
				  int size) {
	
	if (socket->use_tls && socket->ssl != NULL) {
		int total = 0;
		while (total < size) {
			int n = SSL_write(socket->ssl, buffer + total, size - total);
			if (n <= 0) {
				int err = SSL_get_error(socket->ssl, n);
				if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
					continue;
				}
				return false;
			}
			total += n;
		}
		return true;
	}

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
	
	if (socket->use_tls && socket->ssl != NULL) {
		int total = 0;
		while (total < size) {
			int n = SSL_read(socket->ssl, buffer + total, size - total);
			if (n <= 0) {
				int err = SSL_get_error(socket->ssl, n);
				if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
					continue;
				}
				if (err == SSL_ERROR_ZERO_RETURN && total > 0) {
					break;
				}
				return false;
			}
			total += n;
		}
		return true;
	}

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
