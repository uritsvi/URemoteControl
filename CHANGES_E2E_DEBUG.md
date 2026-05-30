# URemoteControl — End‑to‑End Encryption, Debug Mode & Local Run

This document describes the changes made to add **application‑level end‑to‑end
(E2E) encryption**, a **debug flag** that keeps the local machine usable while
testing, **auto windowed (non‑full‑screen) startup**, and the supporting work to
make the whole project (Go relay server + two C clients) run on a single
machine.

---

## 1. End‑to‑end encryption (C clients ⇄ C clients, via the Go relay)

### What "end‑to‑end" means here
The Go server is a **blind relay**: for the screen and input tunnels it reads a
length‑prefixed frame from one client and forwards the exact bytes to the other
(`Server/Tunnel/Tunnel.go`). Because of that, encrypting the payload *inside the
C clients* gives true end‑to‑end secrecy — **the server only ever sees
ciphertext and cannot read or modify the screen or input data**. This is a
stronger property than the pre‑existing TLS support (which only protects each
hop *to the server*, where the server could see plaintext).

Both layers can be used together (TLS for transport + E2E for the payload).

### Cipher & key agreement
- **AES‑256‑GCM** (authenticated encryption) via OpenSSL's EVP API. GCM provides
  confidentiality **and** integrity/authentication (tampered or corrupted frames
  fail to decrypt and are rejected).
- The 256‑bit session key is established by an **ephemeral Diffie‑Hellman key
  exchange (X25519 ECDH)**: at session start each client generates a fresh key
  pair, sends its **public** key to the relay **as part of the connection
  handshake**, and combines its own **private** key with the peer's public key
  (relayed back) to compute a shared secret. The session key is then derived from
  that secret with **HKDF‑SHA256**. Because the private keys never leave the
  machines and a new pair is generated every run, the channel has **forward
  secrecy** and the relay only ever sees public keys (not secret) and ciphertext.
- The optional pre‑shared passphrase (`config.ini` `e2e_key`) is no longer the
  key itself: it **gates** the feature and is **mixed into the HKDF `info`** as an
  authentication binding, so an active relay that tries to man‑in‑the‑middle the
  exchange (by substituting its own public keys) cannot derive a matching session
  key. An empty passphrase disables E2E (plaintext, as before).
- A fresh **random 96‑bit nonce** is generated per frame (`RAND_bytes`).

### Key‑exchange handshake (part of the protocol init data)
The public‑key swap is built into the connection protocol rather than bolted on
afterwards. The relay carries it:

1. **Connect handshake (client → server).** `connect_to_server` appends the
   client's X25519 public key as the last field of the init data it already
   sends (`max_buffer_size`, `client_type`, `target`, `client_index`,
   `notify_on_data_received`, **`public_key`**). The blob is plaintext
   `[uint32 len][len bytes]`, with `len = 0` when E2E is off so the layout stays
   uniform. The Go relay reads it in `handleHandshake` and stores it on the
   client (`Client.PublicKey`).
2. **Key delivery (server → client).** Once all six connections are in, the
   relay sends each control channel its **peer's** public key (a client that
   offered no key is skipped). This happens in `onAllConnected`, right after the
   `ALL_CLIENTS_CONNECTED` message and before the data tunnels start, via
   `ControlChannel.SendPublicKeys`.
3. **Derive (client).** The control‑channel reader (`control_chanel.c`
   `_thread_proc`) reads the peer key immediately after the
   `ALL_CLIENTS_CONNECTED` message and derives the session key
   (`network_receive_peer_key` → `crypto_derive_session_key`) **before** invoking
   the app callback that starts the screen/input threads. So the key is ready
   before any data flows.

The relay handling public keys does **not** weaken E2E: public keys are not
secret, and without a private key the relay still cannot compute the shared
secret (and the passphrase mixed into HKDF authenticates the exchange).

### Sealed frame wire format
```
[ uint32 length ][ 12‑byte nonce ][ ciphertext (= plaintext length) ][ 16‑byte GCM tag ]
```
The `uint32 length` prefix is what the relay reads/forwards, so the **data‑tunnel**
protocol is unchanged — the server forwards opaque ciphertext and never decrypts.
(The relay does now carry the public‑key swap during connection setup, see the
handshake section above, but it only relays keys; it cannot read the payload.)

### What is encrypted
The two data tunnels that carry user data:
- **Screen frames** controlled → controller (`[DeltaPacketInfo][zlib data]`).
- **Input events** controller → controlled (`InputStruct`).

The control channel only carries a single status byte (`ALL_CLIENTS_CONNECTED`)
and is not user data, so it is left as‑is.

### Backwards compatible
If `e2e_key` is empty, the handshake is skipped and `send_encrypted_data` /
`receive_encrypted_data` fall back to the original plaintext
`send_data_with_size` / `receive_data` framing, so the data‑frame wire format is
byte‑for‑byte identical to before. When E2E is on, the only addition to the wire
is the one‑time public‑key blob at the start of each data tunnel.

### Files
| File | Change |
|------|--------|
| `Platform/include/crypto.h` | E2E API: `crypto_configure`, `crypto_is_configured`, `crypto_generate_keypair`, `crypto_export_public_key`, `crypto_derive_session_key`, `crypto_is_enabled`, `crypto_seal`, `crypto_open`. |
| `Platform/src/windows/crypto.c` | X25519 ECDH key agreement + HKDF‑SHA256 session‑key derivation, AES‑256‑GCM seal/open (OpenSSL EVP / EVP_KDF). Per‑process `client_e2e_<pid>.log` with key‑id fingerprint + self‑test. |
| `Platform/include/network.h` | Declares `send_encrypted_data` / `receive_encrypted_data`, plus `network_send_public_key` / `network_receive_peer_key`. |
| `Platform/src/windows/network.c` | Implements the encrypted helpers and the public‑key handshake blob send/receive (`network_send_public_key` → export+send our key; `network_receive_peer_key` → receive peer key + derive session key). |
| `Helpers/src/connect_to_server.c` | Appends `network_send_public_key(socket)` to the connection handshake init data. |
| `Helpers/include/control_chanel.c` | After `ALL_CLIENTS_CONNECTED`, reads the relayed peer key and derives the session key (`network_receive_peer_key`) before the app callback runs. |
| `ControlledApp/main.c`, `ControllerApp/main.c` | `crypto_configure(config->e2e_key)` + `crypto_generate_keypair()` during init. |
| `ControllerApp/app.c` | Enables `crypto_log_enable`. (No per‑app key‑exchange code — it now lives in the connect handshake + control channel.) |
| `Server/Client/Client.go` | Adds `PublicKey []byte` to the client. |
| `Server/SocketWrapper/SocketWarraper.go` | Adds `ReadNBytes`. |
| `Server/ListenForClients/ListenForClients.go` | Reads the public‑key blob as the last handshake field. |
| `Server/ControlChannel/ControlTunnel.go` | `SendPublicKeys` delivers each client its peer's key over the control channel. |
| `Server/ServerMain/ServerMain.go` | Calls `SendPublicKeys` in `onAllConnected` (after the all‑connected message, before tunnels start). |

> The Go server forwards the encrypted **data** tunnels blindly — it never
> decrypts. Its only E2E role is to relay the **public** keys during connection
> setup (it cannot derive the session key without a private key), so true
> end‑to‑end secrecy is preserved. (A separate robustness fix to the server is
> described in §5.)

---

## 2. Debug flag (`debug_mode`)

A single config switch, `debug_mode=1` in `config.ini`, makes it safe to run and
test the software **without losing control of the local mouse and keyboard** —
including running both clients on one machine.

When `debug_mode` is on:
- **Controller does not capture or forward input.** `init_input` (global
  low‑level keyboard/mouse hooks **and** the `ClipCursor` that traps the pointer
  in the window) is **skipped**, and the input‑forwarding callback / monitor
  control keys are not registered. → your mouse and keyboard stay free.
- **Controlled never applies remote input.** The input apply loop returns early
  before `SendInput`, so nothing can move your real cursor or type for you.
- **Controller window opens windowed** (see §3).
- The remote screen is still received and displayed, so you can confirm the
  pipeline works.

### Files
| File | Change |
|------|--------|
| `Common/Common.h` | New INI keys `INI_DEBUG_MODE` (`debug_mode`) and `INI_E2E_KEY` (`e2e_key`). |
| `Helpers/include/program_config.h` | New `bool debug_mode;` and `char e2e_key[DEFAULT_BUFFER_SIZE];` fields. |
| `Platform/src/windows/read_ini.c` | Parses `e2e_key` (default empty) and `debug_mode` (default `0`). |
| `ControllerApp/app.c` | Skips `init_input`, cursor reset, control keys and input callback in debug mode; still receives/draws the screen and connects all tunnels. Sets windowed startup. |
| `ControlledApp/app.c` | Skips applying remote input in debug mode. |

---

## 3. Auto non‑full‑screen (windowed) startup

Previously `show_window` always forced the borderless full‑screen popup style
(`WS_POPUPWINDOW`). Now the controller can open in a normal titled, movable
window.

- `Platform/include/window.h` / `Platform/src/windows/window.c`: new
  `set_window_start_windowed(bool)`. When set, `show_window` keeps the overlapped
  windowed style instead of the full‑screen popup.
- `ControllerApp/app.c`: calls `set_window_start_windowed(config->debug_mode)`
  before `show_window`, so debug mode → windowed.
- The existing **Ctrl+Space** hotkey still toggles full‑screen at runtime.

---

## 4. Complete `config.ini`

`read_ini` *exits* if a required numeric field is missing, and the previous
`bin/config.ini` only contained the input‑control flags. A complete config is
now shipped at `bin/config.ini`:

```ini
[GuiInfo]
mac_address=74563ce2
password=60b1d8

[config]
target_width=1280
target_height=720
target_bit_count=16          ; matches the capture pipeline's 2‑bytes/pixel math
compression_level=1
max_send_buffer=65536
num_of_delta_parts=10
capture_full_screen_interval=1000   ; full‑frame refresh timer (ms)
use_tls=0
tls_server_name=
tls_ca_cert_path=
allow_remote_keyboard_control=0
allow_remote_mouse_control=0
e2e_key=URemoteControl-demo-shared-secret-change-me   ; shared E2E passphrase
debug_mode=1                 ; safe local testing
```

`window_width`/`window_height` are derived from `target_width`/`target_height`
by `read_ini` (no separate keys).

**To use it for real remote control:** set `e2e_key` to the *same* strong secret
on both machines, set `debug_mode=0`, and (optionally) set
`allow_remote_keyboard_control` / `allow_remote_mouse_control` to `1` on the
controlled machine.

---

## 5. Server robustness fix

`Server/ListenForClients/ListenForClients.go` previously **panicked and killed
the whole server** if any connection closed before completing the handshake
(e.g. a TCP port probe). The accept loop now handles each handshake in an
isolated function with `recover()`: a malformed/aborted connection is logged and
dropped, and the server keeps accepting. This makes the relay resilient to
probes and flaky clients.

---

## 6. Build & run

### Build
```powershell
# C/C++ clients (uses vcpkg openssl + zlib, VS2022 build tools)
powershell -ExecutionPolicy Bypass -File .\scripts\build-c-apps.ps1 -Configuration Debug
# Go server builds on the fly via `go run`, or: cd Server; go build ./...
```

### Run the whole stack locally (server + both clients) in debug mode
```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run-debug.ps1 -BuildFirst
```
`scripts/run-debug.ps1` *(new)*:
- starts the Go relay server (output → `logs/go-server.log`),
- waits until it logs that it is listening,
- starts `ControlledApp` then `ControllerApp` (both `127.0.0.1 47800` by
  default, debug mode from `config.ini`),
- waits for the server to log **"All clients are connected"** and reports
  per‑process status. Use `-KeepOpen` to keep monitoring until exit.

Stop everything:
```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\stop-debug.ps1
```

---

## 7. Security notes
- The session key now comes from an **ephemeral X25519 Diffie‑Hellman exchange**,
  so it is unique per run and never derived from (or transmitted as) a static
  secret → **forward secrecy**: compromising the passphrase later does not
  decrypt previously captured sessions.
- **Set the same `e2e_key` on both peers.** It no longer *is* the key, but it
  **authenticates** the exchange (mixed into HKDF), which is what stops an active
  relay from man‑in‑the‑middling the otherwise anonymous DH. A high‑entropy value
  is still recommended; an empty value means anonymous DH (encrypted against a
  passive relay only) — and in this build an empty value disables E2E entirely.
- AES‑GCM with a random 96‑bit nonce per message is safe for the message volumes
  here; the per‑frame fresh nonce avoids nonce reuse.
- E2E and TLS are independent and composable: TLS protects the hop to the server,
  E2E ensures the server itself can never read the payload.
- **Verification:** each client writes `bin/client_e2e_<pid>.log`. A successful
  run shows the **same `key_id`** on both peers (proving the DH agreed on one
  key) and `GCM auth OK` on every opened frame.
