#ifndef __CRYPTO__
#define __CRYPTO__

#include <stdbool.h>
#include <stdint.h>

/*
 * Application level end-to-end encryption helpers.
 *
 * Key agreement is done with an ephemeral Diffie-Hellman exchange: at session
 * start each client generates a fresh X25519 key pair, sends its PUBLIC key to
 * the peer through the (blind) relay server, and combines its own PRIVATE key
 * with the peer's public key to compute a shared secret. The 256 bit AES-GCM
 * session key is then derived from that secret with HKDF-SHA256. Because the
 * private keys never leave the machines and a new pair is generated per run,
 * the channel has forward secrecy and the relay only ever sees public keys and
 * ciphertext.
 *
 * An optional pre-shared passphrase (config.ini "e2e_key") gates the feature
 * and is mixed into the HKDF as an authentication binding, so an active relay
 * that tries to man-in-the-middle the exchange (by substituting its own public
 * keys) cannot derive a matching session key. An empty passphrase disables
 * end-to-end encryption entirely (the network layer then sends plaintext).
 *
 * Wire format of a sealed blob:
 *   [ 12 byte nonce ][ ciphertext (= plaintext length) ][ 16 byte GCM tag ]
 */

/* nonce (12) + GCM tag (16) */
#define CRYPTO_NONCE_SIZE 12
#define CRYPTO_TAG_SIZE 16
#define CRYPTO_OVERHEAD (CRYPTO_NONCE_SIZE + CRYPTO_TAG_SIZE)

/* Raw X25519 public key length (the value exchanged during the handshake). */
#define CRYPTO_PUBLIC_KEY_SIZE 32

/*
 * Configure end-to-end encryption from the pre-shared passphrase. This only
 * records whether the feature is on and the authentication secret to bind into
 * the key derivation; the actual session key is established later by the
 * Diffie-Hellman handshake (crypto_generate_keypair + crypto_derive_session_key).
 * A NULL or empty passphrase disables end-to-end encryption.
 */
void crypto_configure(const char* passphrase);

/* true when a non empty passphrase was configured (handshake should run). */
bool crypto_is_configured(void);

/*
 * true once a session key has been established (crypto_derive_session_key
 * succeeded). The network layer seals/opens payloads only when this is true,
 * otherwise it falls back to plaintext.
 */
bool crypto_is_enabled(void);

/*
 * Generate this process's ephemeral X25519 key pair. Must be called before
 * crypto_export_public_key / crypto_derive_session_key. Returns false on error.
 */
bool crypto_generate_keypair(void);

/*
 * Copy this process's raw public key into out (CRYPTO_PUBLIC_KEY_SIZE bytes).
 * Returns the number of bytes written, or -1 on error.
 */
int crypto_export_public_key(unsigned char* out, int out_cap);

/*
 * Combine our private key with the peer's public key (X25519 ECDH) and derive
 * the AES-256-GCM session key from the shared secret with HKDF-SHA256 (the
 * configured passphrase is mixed in as an authentication binding). On success
 * encryption is enabled (crypto_is_enabled becomes true). Returns false on
 * error.
 */
bool crypto_derive_session_key(const unsigned char* peer_pub, int peer_pub_len);

/*
 * Enable end-to-end-encryption proof logging for this process: writes a status
 * header to the debug console and to client_e2e.log next to the executable.
 * Once the session key is established it also logs the key fingerprint and runs
 * a self-test (round-trip + tamper-rejection), then samples per-frame
 * seal/open events.
 */
void crypto_log_enable(void);

/*
 * Seal plaintext into out. out must have room for pt_len + CRYPTO_OVERHEAD
 * bytes. Returns the number of bytes written, or -1 on error.
 */
int crypto_seal(
	const unsigned char* pt,
	int pt_len,
	unsigned char* out,
	int out_cap);

/*
 * Open a sealed blob into out. out must have room for in_len - CRYPTO_OVERHEAD
 * bytes. Returns the plaintext length, or -1 on error (including a failed
 * authentication tag check).
 */
int crypto_open(
	const unsigned char* in,
	int in_len,
	unsigned char* out,
	int out_cap);

#endif
