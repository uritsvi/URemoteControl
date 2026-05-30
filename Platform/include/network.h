#ifndef __NETWORK__
#define __NETWORK__

#include <stdint.h>

#include <stdbool.h>

typedef int Socket;

void network_set_tls_config(bool use_tls, const char* server_name, const char* ca_cert_path);

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

/*
 * End-to-end encrypted, length-prefixed payload helpers. When an e2e key has
 * been configured (see crypto_set_key) the payload is sealed with AES-256-GCM;
 * otherwise these fall back to the plaintext send_data_with_size / receive_data
 * behaviour so the wire format stays identical.
 */
bool send_encrypted_data(
	Socket socket,
	char* buffer,
	int size);

bool receive_encrypted_data(
	Socket socket,
	char* buffer,
	int* size);

/*
 * End-to-end-encryption key exchange, carried as part of the connection
 * protocol's init data. Public keys are not secret, so these frames are sent
 * in plaintext as a length-prefixed blob: [ uint32 length ][ length bytes ].
 *
 * network_send_public_key is called during the connect handshake: it sends our
 * ephemeral X25519 public key to the relay (a zero-length blob when E2E is not
 * configured, so the wire layout stays uniform).
 *
 * network_receive_peer_key is called on the control channel once the relay has
 * delivered the peer's public key: it reads the blob and derives the shared
 * AES-256-GCM session key. Both are no-ops (return true) when E2E is off.
 */
bool network_send_public_key(Socket socket);

bool network_receive_peer_key(Socket socket);

#endif