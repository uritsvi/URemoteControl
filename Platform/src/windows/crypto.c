#ifdef _WIN32

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <share.h>
#include <process.h>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/kdf.h>
#include <openssl/core_names.h>
#include <openssl/params.h>

#include <Common.h>
#include <platform.h>

#include "crypto.h"

#pragma comment(lib, "libcrypto.lib")

/*
 * Fixed application salt for the HKDF key derivation. A fixed salt is fine here:
 * the entropy comes from the per-session ephemeral Diffie-Hellman secret, and
 * the salt only domain-separates this application from others.
 */
static const unsigned char g_Salt[] = "URemoteControl-e2e-v2";

/*
 * HKDF "info" label. The optional pre-shared passphrase is appended after this
 * label (see crypto_derive_session_key) so two peers only derive the same key
 * if they share the passphrase - this authenticates the otherwise anonymous
 * Diffie-Hellman exchange against an active man-in-the-middle relay.
 */
static const char g_InfoLabel[] = "URemoteControl-e2e-session-key-v2";

#define CRYPTO_KEY_SIZE 32 /* AES-256 */

static bool g_Configured = false;
static bool g_Enabled = false;
static unsigned char g_Key[CRYPTO_KEY_SIZE];

/* Optional pre-shared authentication secret (copied from config e2e_key). */
static char g_Passphrase[DEFAULT_BUFFER_SIZE] = { 0 };

/* This process's ephemeral X25519 key pair (private + public). */
static EVP_PKEY* g_KeyPair = NULL;

/* E2E proof logging (opt-in per process via crypto_log_enable). */
static bool g_LogWanted = false;
static bool g_LogEnabled = false;
static FILE* g_LogFile = NULL;
static unsigned long g_SealCount = 0;
static unsigned long g_OpenCount = 0;

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
	EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
	if (pctx == NULL) {
		ERR_print_errors_fp(stderr);
		return false;
	}

	bool ok = false;
	EVP_PKEY* pkey = NULL;

	if (EVP_PKEY_keygen_init(pctx) != 1) {
		ERR_print_errors_fp(stderr);
		goto done;
	}
	if (EVP_PKEY_keygen(pctx, &pkey) != 1) {
		ERR_print_errors_fp(stderr);
		goto done;
	}

	if (g_KeyPair != NULL) {
		EVP_PKEY_free(g_KeyPair);
	}
	g_KeyPair = pkey;
	ok = true;

done:
	EVP_PKEY_CTX_free(pctx);
	return ok;
}

int crypto_export_public_key(unsigned char* out, int out_cap) {
	if (g_KeyPair == NULL || out_cap < CRYPTO_PUBLIC_KEY_SIZE) {
		return -1;
	}

	size_t len = (size_t)out_cap;
	if (EVP_PKEY_get_raw_public_key(g_KeyPair, out, &len) != 1) {
		ERR_print_errors_fp(stderr);
		return -1;
	}

	return (int)len;
}

/*
 * HKDF-SHA256(ikm=secret, salt=g_Salt, info=g_InfoLabel || passphrase) -> g_Key.
 */
static bool _derive_key_from_secret(
	const unsigned char* secret,
	size_t secret_len) {

	/* info = label (without its NUL) followed by the passphrase. */
	unsigned char info[sizeof(g_InfoLabel) - 1 + DEFAULT_BUFFER_SIZE];
	size_t label_len = sizeof(g_InfoLabel) - 1;
	size_t pass_len = strlen(g_Passphrase);

	memcpy(info, g_InfoLabel, label_len);
	memcpy(info + label_len, g_Passphrase, pass_len);
	size_t info_len = label_len + pass_len;

	EVP_KDF* kdf = EVP_KDF_fetch(NULL, "HKDF", NULL);
	if (kdf == NULL) {
		ERR_print_errors_fp(stderr);
		return false;
	}

	EVP_KDF_CTX* kctx = EVP_KDF_CTX_new(kdf);
	EVP_KDF_free(kdf);
	if (kctx == NULL) {
		ERR_print_errors_fp(stderr);
		return false;
	}

	OSSL_PARAM params[5];
	params[0] = OSSL_PARAM_construct_utf8_string(
		OSSL_KDF_PARAM_DIGEST, "SHA256", 0);
	params[1] = OSSL_PARAM_construct_octet_string(
		OSSL_KDF_PARAM_KEY, (void*)secret, secret_len);
	params[2] = OSSL_PARAM_construct_octet_string(
		OSSL_KDF_PARAM_SALT, (void*)g_Salt, sizeof(g_Salt) - 1);
	params[3] = OSSL_PARAM_construct_octet_string(
		OSSL_KDF_PARAM_INFO, info, info_len);
	params[4] = OSSL_PARAM_construct_end();

	int res = EVP_KDF_derive(kctx, g_Key, CRYPTO_KEY_SIZE, params);
	EVP_KDF_CTX_free(kctx);

	if (res != 1) {
		ERR_print_errors_fp(stderr);
		return false;
	}

	return true;
}

/* Log the session key fingerprint and run the seal/open self-test. */
static void _log_session_established(void) {
	unsigned char digest[SHA256_DIGEST_LENGTH];
	SHA256(g_Key, CRYPTO_KEY_SIZE, digest);
	char key_id[4 * 2 + 1];
	_to_hex(digest, 4, key_id);
	_e2e_logf(
		"[E2E] session key established  cipher=AES-256-GCM  kex=X25519-ECDH  kdf=HKDF-SHA256  key_id=%s\n",
		key_id);
	_e2e_logf("[E2E] (key_id = first 4 bytes of SHA-256 of the session key; both peers must match)\n");

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

		/* Flip one ciphertext byte: GCM authentication must reject it. */
		sealed[CRYPTO_NONCE_SIZE] ^= 0x01;
		int tlen = crypto_open(sealed, slen, opened, (int)sizeof(opened));
		_e2e_logf("[E2E] SELF-TEST tamper rejected by GCM auth: %s\n", (tlen < 0) ? "PASS" : "FAIL");
	}

	/* Start per-frame logging fresh after the self-test. */
	g_SealCount = 0;
	g_OpenCount = 0;
	g_LogEnabled = true;
	_e2e_logf("[E2E] live logging enabled; sampling 1st frame then every 100th.\n");
}

bool crypto_derive_session_key(const unsigned char* peer_pub, int peer_pub_len) {
	if (g_KeyPair == NULL || peer_pub == NULL || peer_pub_len <= 0) {
		return false;
	}

	EVP_PKEY* peer = EVP_PKEY_new_raw_public_key(
		EVP_PKEY_X25519, NULL, peer_pub, (size_t)peer_pub_len);
	if (peer == NULL) {
		ERR_print_errors_fp(stderr);
		return false;
	}

	EVP_PKEY_CTX* dctx = EVP_PKEY_CTX_new(g_KeyPair, NULL);
	if (dctx == NULL) {
		ERR_print_errors_fp(stderr);
		EVP_PKEY_free(peer);
		return false;
	}

	bool ok = false;
	unsigned char secret[CRYPTO_PUBLIC_KEY_SIZE];
	size_t secret_len = sizeof(secret);

	if (EVP_PKEY_derive_init(dctx) != 1) {
		ERR_print_errors_fp(stderr);
		goto done;
	}
	if (EVP_PKEY_derive_set_peer(dctx, peer) != 1) {
		ERR_print_errors_fp(stderr);
		goto done;
	}
	if (EVP_PKEY_derive(dctx, secret, &secret_len) != 1) {
		ERR_print_errors_fp(stderr);
		goto done;
	}

	if (!_derive_key_from_secret(secret, secret_len)) {
		goto done;
	}

	g_Enabled = true;
	ok = true;

	if (g_LogWanted) {
		_log_session_established();
	}

done:
	OPENSSL_cleanse(secret, sizeof(secret));
	EVP_PKEY_CTX_free(dctx);
	EVP_PKEY_free(peer);
	return ok;
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
	unsigned char* tag = out + CRYPTO_NONCE_SIZE + pt_len;

	if (RAND_bytes(nonce, CRYPTO_NONCE_SIZE) != 1) {
		return -1;
	}

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (ctx == NULL) {
		return -1;
	}

	int result = -1;
	int len = 0;
	int ciphertext_len = 0;

	if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1) {
		goto done;
	}
	if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, CRYPTO_NONCE_SIZE, NULL) != 1) {
		goto done;
	}
	if (EVP_EncryptInit_ex(ctx, NULL, NULL, g_Key, nonce) != 1) {
		goto done;
	}

	if (EVP_EncryptUpdate(ctx, ciphertext, &len, pt, pt_len) != 1) {
		goto done;
	}
	ciphertext_len = len;

	if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1) {
		goto done;
	}
	ciphertext_len += len;

	if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, CRYPTO_TAG_SIZE, tag) != 1) {
		goto done;
	}

	result = CRYPTO_NONCE_SIZE + ciphertext_len + CRYPTO_TAG_SIZE;

	if (g_LogEnabled) {
		g_SealCount++;
		if (g_SealCount == 1 || (g_SealCount % 100) == 0) {
			int show = ciphertext_len < 16 ? ciphertext_len : 16;
			char nonce_hex[CRYPTO_NONCE_SIZE * 2 + 1];
			char ct_hex[16 * 2 + 1];
			_to_hex(nonce, CRYPTO_NONCE_SIZE, nonce_hex);
			_to_hex(ciphertext, show, ct_hex);
			_e2e_logf(
				"[E2E] seal #%lu: plaintext=%d B -> ciphertext=%d B (nonce+ct+tag), nonce=%s, cipher[0..%d]=%s\n",
				g_SealCount, pt_len, result, nonce_hex, show, ct_hex);
		}
	}

done:
	EVP_CIPHER_CTX_free(ctx);
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

	int pt_len = in_len - CRYPTO_OVERHEAD;
	if (out_cap < pt_len) {
		return -1;
	}

	const unsigned char* nonce = in;
	const unsigned char* ciphertext = in + CRYPTO_NONCE_SIZE;
	unsigned char* tag = (unsigned char*)(in + CRYPTO_NONCE_SIZE + pt_len);

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (ctx == NULL) {
		return -1;
	}

	int result = -1;
	int len = 0;
	int plaintext_len = 0;

	if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1) {
		goto done;
	}
	if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, CRYPTO_NONCE_SIZE, NULL) != 1) {
		goto done;
	}
	if (EVP_DecryptInit_ex(ctx, NULL, NULL, g_Key, nonce) != 1) {
		goto done;
	}

	if (EVP_DecryptUpdate(ctx, out, &len, ciphertext, pt_len) != 1) {
		goto done;
	}
	plaintext_len = len;

	if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, CRYPTO_TAG_SIZE, tag) != 1) {
		goto done;
	}

	/* Returns <= 0 if the authentication tag does not verify. */
	if (EVP_DecryptFinal_ex(ctx, out + len, &len) <= 0) {
		goto done;
	}
	plaintext_len += len;

	result = plaintext_len;

	if (g_LogEnabled) {
		g_OpenCount++;
		if (g_OpenCount == 1 || (g_OpenCount % 100) == 0) {
			_e2e_logf(
				"[E2E] open #%lu: ciphertext=%d B -> plaintext=%d B, GCM auth OK\n",
				g_OpenCount, in_len, result);
		}
	}

done:
	EVP_CIPHER_CTX_free(ctx);
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
		"[E2E] status: ENABLED  key-exchange=X25519 ECDH  kdf=HKDF-SHA256  cipher=AES-256-GCM\n");
	_e2e_logf("[E2E] waiting for Diffie-Hellman handshake to establish the session key...\n");

	/* The fingerprint + self-test are logged when the session key is derived. */
	g_LogWanted = true;
}

#endif
