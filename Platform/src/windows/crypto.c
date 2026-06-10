#ifdef _WIN32

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <share.h>
#include <process.h>

#include <sodium.h>

#include <Common.h>
#include <platform.h>

#include "crypto.h"

#pragma comment(lib, "libsodium.lib")

/*
 * Domain-separation label for the BLAKE2b key derivation. It is hashed together
 * with the shared secret and the pre-shared passphrase so two peers only derive
 * the same key if they share the passphrase - this authenticates the otherwise
 * anonymous Diffie-Hellman exchange against an active man-in-the-middle relay.
 */
static const char g_InfoLabel[] = "URemoteControl-e2e-session-key-v3";

#define CRYPTO_KEY_SIZE crypto_aead_xchacha20poly1305_ietf_KEYBYTES /* 32 */

static bool g_Configured = false;
static bool g_Enabled = false;
static unsigned char g_Key[CRYPTO_KEY_SIZE];

/* Optional pre-shared authentication secret (copied from config e2e_key). */
static char g_Passphrase[DEFAULT_BUFFER_SIZE] = { 0 };

/* This process's ephemeral X25519 key pair. */
static unsigned char g_PublicKey[crypto_box_PUBLICKEYBYTES];
static unsigned char g_SecretKey[crypto_box_SECRETKEYBYTES];

/* E2E proof logging (opt-in per process via crypto_log_enable). */
static bool g_LogWanted = false;
static bool g_LogEnabled = false;
static FILE* g_LogFile = NULL;
static unsigned long g_SealCount = 0;
static unsigned long g_OpenCount = 0;

/* sodium_init must run once before any libsodium call; returns false on error. */
static bool _ensure_sodium(void) {
	static bool initialized = false;
	if (initialized) {
		return true;
	}
	if (sodium_init() < 0) {
		return false;
	}
	initialized = true;
	return true;
}

static void _to_hex(const unsigned char* in, int n, char* out) {
	static const char* hexd = "0123456789abcdef";
	for (int i = 0; i < n; i++) {
		out[i * 2] = hexd[(in[i] >> 4) & 0xF];
		out[i * 2 + 1] = hexd[in[i] & 0xF];
	}
	out[n * 2] = '\0';
}

/* Write to both the debug console (stdout) and the e2e log file. */
static void _e2e_logf(const char* fmt, ...) {
	va_list args;

	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);

	if (g_LogFile != NULL) {
		va_start(args, fmt);
		vfprintf(g_LogFile, fmt, args);
		va_end(args);
		fflush(g_LogFile);
	}
}

void crypto_configure(const char* passphrase) {
	g_Enabled = false;

	if (passphrase == NULL || passphrase[0] == '\0') {
		g_Configured = false;
		g_Passphrase[0] = '\0';
		return;
	}

	strncpy_s(g_Passphrase, DEFAULT_BUFFER_SIZE, passphrase, _TRUNCATE);
	g_Configured = true;
}

bool crypto_is_configured(void) {
	return g_Configured;
}

bool crypto_is_enabled(void) {
	return g_Enabled;
}

bool crypto_generate_keypair(void) {
	if (!_ensure_sodium()) {
		return false;
	}

	/* crypto_box keys are X25519 key pairs. */
	crypto_box_keypair(g_PublicKey, g_SecretKey);
	return true;
}

int crypto_export_public_key(unsigned char* out, int out_cap) {
	if (out_cap < (int)sizeof(g_PublicKey)) {
		return -1;
	}

	memcpy(out, g_PublicKey, sizeof(g_PublicKey));
	return (int)sizeof(g_PublicKey);
}

/*
 * BLAKE2b(label || shared_secret || passphrase) -> g_Key. Folding the label and
 * passphrase into the hash mirrors HKDF's salt/info binding: the passphrase
 * authenticates the exchange, and the label domain-separates this application.
 */
static bool _derive_key_from_secret(
	const unsigned char* secret,
	size_t secret_len) {

	crypto_generichash_state st;
	crypto_generichash_init(&st, NULL, 0, CRYPTO_KEY_SIZE);
	crypto_generichash_update(&st, (const unsigned char*)g_InfoLabel, sizeof(g_InfoLabel) - 1);
	crypto_generichash_update(&st, secret, secret_len);
	crypto_generichash_update(&st, (const unsigned char*)g_Passphrase, strlen(g_Passphrase));
	crypto_generichash_final(&st, g_Key, CRYPTO_KEY_SIZE);
	return true;
}

/* Log the session key fingerprint and run the seal/open self-test. */
static void _log_session_established(void) {
	unsigned char digest[crypto_generichash_BYTES];
	crypto_generichash(digest, sizeof(digest), g_Key, CRYPTO_KEY_SIZE, NULL, 0);
	char key_id[4 * 2 + 1];
	_to_hex(digest, 4, key_id);
	_e2e_logf(
		"[E2E] session key established  cipher=XChaCha20-Poly1305  kex=X25519-ECDH  kdf=BLAKE2b  key_id=%s\n",
		key_id);
	_e2e_logf("[E2E] (key_id = first 4 bytes of BLAKE2b of the session key; both peers must match)\n");

	/* --- self test: prove seal/open actually work with the derived key --- */
	const char* msg = "URemoteControl E2E self-test payload";
	int mlen = (int)strlen(msg);

	unsigned char sealed[128];
	unsigned char opened[128];

	int slen = crypto_seal((const unsigned char*)msg, mlen, sealed, (int)sizeof(sealed));
	if (slen < 0) {
		_e2e_logf("[E2E] SELF-TEST: seal FAILED\n");
	} else {
		int show = slen < 32 ? slen : 32;
		char ct_hex[32 * 2 + 1];
		_to_hex(sealed, show, ct_hex);
		_e2e_logf("[E2E] SELF-TEST plaintext (%d B): \"%s\"\n", mlen, msg);
		_e2e_logf("[E2E] SELF-TEST sealed   (%d B), first %d bytes: %s\n", slen, show, ct_hex);

		int olen = crypto_open(sealed, slen, opened, (int)sizeof(opened));
		bool roundtrip = (olen == mlen) && (memcmp(opened, msg, mlen) == 0);
		_e2e_logf("[E2E] SELF-TEST decrypt round-trip: %s\n", roundtrip ? "PASS" : "FAIL");

		/* Flip one ciphertext byte: Poly1305 authentication must reject it. */
		sealed[CRYPTO_NONCE_SIZE] ^= 0x01;
		int tlen = crypto_open(sealed, slen, opened, (int)sizeof(opened));
		_e2e_logf("[E2E] SELF-TEST tamper rejected by AEAD auth: %s\n", (tlen < 0) ? "PASS" : "FAIL");
	}

	/* Start per-frame logging fresh after the self-test. */
	g_SealCount = 0;
	g_OpenCount = 0;
	g_LogEnabled = true;
	_e2e_logf("[E2E] live logging enabled; sampling 1st frame then every 100th.\n");
}

bool crypto_derive_session_key(const unsigned char* peer_pub, int peer_pub_len) {
	if (peer_pub == NULL || peer_pub_len != crypto_box_PUBLICKEYBYTES) {
		return false;
	}

	unsigned char secret[crypto_scalarmult_BYTES];

	/* X25519 ECDH: shared = scalarmult(our private, peer public). */
	if (crypto_scalarmult(secret, g_SecretKey, peer_pub) != 0) {
		return false;
	}

	_derive_key_from_secret(secret, sizeof(secret));
	sodium_memzero(secret, sizeof(secret));

	g_Enabled = true;

	if (g_LogWanted) {
		_log_session_established();
	}

	return true;
}

int crypto_seal(
	const unsigned char* pt,
	int pt_len,
	unsigned char* out,
	int out_cap) {

	if (!g_Enabled) {
		return -1;
	}

	if (pt_len < 0 || out_cap < pt_len + CRYPTO_OVERHEAD) {
		return -1;
	}

	unsigned char* nonce = out;
	unsigned char* ciphertext = out + CRYPTO_NONCE_SIZE;

	randombytes_buf(nonce, CRYPTO_NONCE_SIZE);

	unsigned long long ct_len = 0;
	if (crypto_aead_xchacha20poly1305_ietf_encrypt(
			ciphertext, &ct_len,
			pt, (unsigned long long)pt_len,
			NULL, 0,          /* no additional authenticated data */
			NULL,             /* no secret nonce */
			nonce, g_Key) != 0) {
		return -1;
	}

	int result = CRYPTO_NONCE_SIZE + (int)ct_len;

	if (g_LogEnabled) {
		g_SealCount++;
		if (g_SealCount == 1 || (g_SealCount % 100) == 0) {
			int show = (int)ct_len < 16 ? (int)ct_len : 16;
			char nonce_hex[CRYPTO_NONCE_SIZE * 2 + 1];
			char ct_hex[16 * 2 + 1];
			_to_hex(nonce, CRYPTO_NONCE_SIZE, nonce_hex);
			_to_hex(ciphertext, show, ct_hex);
			_e2e_logf(
				"[E2E] seal #%lu: plaintext=%d B -> ciphertext=%d B (nonce+ct+tag), nonce=%s, cipher[0..%d]=%s\n",
				g_SealCount, pt_len, result, nonce_hex, show, ct_hex);
		}
	}

	return result;
}

int crypto_open(
	const unsigned char* in,
	int in_len,
	unsigned char* out,
	int out_cap) {

	if (!g_Enabled) {
		return -1;
	}

	if (in_len < CRYPTO_OVERHEAD) {
		return -1;
	}

	int pt_cap = in_len - CRYPTO_NONCE_SIZE;
	if (out_cap < in_len - CRYPTO_OVERHEAD) {
		return -1;
	}

	const unsigned char* nonce = in;
	const unsigned char* ciphertext = in + CRYPTO_NONCE_SIZE;

	unsigned long long pt_len = 0;
	if (crypto_aead_xchacha20poly1305_ietf_decrypt(
			out, &pt_len,
			NULL,             /* no secret nonce */
			ciphertext, (unsigned long long)pt_cap,
			NULL, 0,          /* no additional authenticated data */
			nonce, g_Key) != 0) {
		/* Authentication tag did not verify (tampered or wrong key). */
		return -1;
	}

	int result = (int)pt_len;

	if (g_LogEnabled) {
		g_OpenCount++;
		if (g_OpenCount == 1 || (g_OpenCount % 100) == 0) {
			_e2e_logf(
				"[E2E] open #%lu: ciphertext=%d B -> plaintext=%d B, AEAD auth OK\n",
				g_OpenCount, in_len, result);
		}
	}

	return result;
}

/*
 * Enable E2E proof logging for this process. Opens the log file and writes the
 * status header. The key fingerprint and the seal/open self-test are written
 * later, once the Diffie-Hellman handshake establishes the session key (see
 * crypto_derive_session_key -> _log_session_established). Output goes to the
 * debug console and to client_e2e.log next to the executable.
 */
void crypto_log_enable(void) {
	if (g_LogFile == NULL) {
		char dir[DEFAULT_BUFFER_SIZE];
		get_runing_dir(dir);

		/*
		 * Tag the log with the process id so the controller and controlled can
		 * each write their own file (they often share a working directory) and
		 * their session-key fingerprints can be compared side by side.
		 */
		char path[DEFAULT_BUFFER_SIZE];
		_snprintf_s(
			path,
			DEFAULT_BUFFER_SIZE,
			_TRUNCATE,
			"%s\\client_e2e_%d.log",
			dir,
			_getpid());

		/* Open with shared read so the log can be tailed while the client runs. */
		g_LogFile = _fsopen(path, "w", _SH_DENYWR);
		_e2e_logf("[E2E] log file: %s\n", path);
	}

	if (!g_Configured) {
		_e2e_logf("[E2E] status: DISABLED (no e2e_key) - payloads are sent in PLAINTEXT\n");
		g_LogWanted = false;
		return;
	}

	_e2e_logf(
		"[E2E] status: ENABLED  key-exchange=X25519 ECDH  kdf=BLAKE2b  cipher=XChaCha20-Poly1305\n");
	_e2e_logf("[E2E] waiting for Diffie-Hellman handshake to establish the session key...\n");

	/* The fingerprint + self-test are logged when the session key is derived. */
	g_LogWanted = true;
}

#endif
