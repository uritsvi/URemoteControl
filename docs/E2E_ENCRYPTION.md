# URemoteControl — End-to-End Encryption: How It Works

This document is a detailed, self-contained explanation of the encryption in
URemoteControl: what it protects, the cryptography it uses, and exactly how the
key exchange is woven into the connection protocol. It complements the
change-log in [`CHANGES_E2E_DEBUG.md`](../CHANGES_E2E_DEBUG.md).

---

## 1. The system in one picture

URemoteControl is a remote-desktop tool with **three** processes:

```
   ControlledApp                 Go relay server                ControllerApp
 (the machine being     <----  (blind byte forwarder)  ---->   (the operator's
  viewed/controlled)                                            screen + input)

  - captures screen  --- screen tunnel (controlled -> controller) -->  displays screen
  - applies input    <-- io tunnel    (controller -> controlled) <--   captures input
  - control channel  <----------- server -> client messages ----------> control channel
```

The two clients never talk directly. Every byte goes through the Go relay,
which **pairs** the two clients and forwards frames between them. Each peer
opens **three** connections to the server:

| Connection (`client_type`)      | Direction through relay        | Carries                         |
|---------------------------------|--------------------------------|---------------------------------|
| `CONTROL_CHANEL_CLIENT_TYPE` (0)| server → client (client reads) | coordination messages           |
| `SYNC_SCREEN_BUFFER_CLIENT_TYPE`(1)| controlled → controller     | screen deltas (`[info][zlib]`)  |
| `SYNC_IO_CLIENT_TYPE` (2)       | controller → controlled        | input events (`InputStruct`)    |

So there are **six** connections in a session (three per client). The server
starts forwarding the data tunnels only after **all six** are connected.

---

## 2. Two independent layers of protection

URemoteControl can apply **two** layers, which compose:

1. **TLS (transport encryption)** — protects each hop *to the server*. Enabled
   with `use_tls=1` in `config.ini`; the server is given a certificate via the
   `UREMOTE_TLS_CERT` / `UREMOTE_TLS_KEY` env vars. With TLS, the link between a
   client and the relay is encrypted, but **the relay terminates TLS and can see
   the plaintext** it forwards.

2. **End-to-end encryption (E2E)** — protects the *payload* between the two
   clients. The clients encrypt screen/input data with a key the **server never
   learns**, so the relay only ever forwards opaque ciphertext. This is the
   stronger property and the subject of this document.

Using both means: TLS hides traffic from the network, and E2E hides the payload
even from the relay itself.

---

## 3. Cryptographic building blocks

All crypto uses **OpenSSL 3.x** (vcpkg `x64-windows`), wrapped in
[`Platform/src/windows/crypto.c`](../Platform/src/windows/crypto.c).

| Purpose                | Algorithm            | Notes                                            |
|------------------------|----------------------|--------------------------------------------------|
| Key agreement          | **X25519 ECDH**      | Ephemeral key pair generated fresh per run       |
| Key derivation         | **HKDF-SHA256**      | Turns the shared secret into a 256-bit AES key   |
| Authenticated encryption| **AES-256-GCM**     | Confidentiality + integrity (tamper detection)   |
| Nonce                  | 96-bit random        | Fresh per frame via `RAND_bytes`                 |
| Authentication binding | pre-shared passphrase| Mixed into HKDF `info` (see §6)                  |

### Why these choices
- **X25519** is a modern, fast, misuse-resistant elliptic-curve Diffie-Hellman.
  Each side keeps a **private** key it never transmits and publishes a 32-byte
  **public** key. Combining your private key with the peer's public key yields a
  shared secret that an eavesdropper (the relay) cannot compute.
- **Ephemeral** key pairs (new every session) give **forward secrecy**:
  recording today's ciphertext and stealing the passphrase tomorrow does not
  decrypt it, because the private keys are already gone.
- **AES-256-GCM** authenticates as well as encrypts: a single flipped bit makes
  decryption fail, so the relay cannot tamper with frames undetected.

---

## 4. The key exchange is part of the connection protocol

The defining design point: the public-key swap is **built into the connection
handshake**, not bolted on afterwards. The relay carries it. There are four
steps.

### Step 0 — generate the key pair (at startup)
In each client's `init()` ([`ControlledApp/main.c`](../ControlledApp/main.c),
[`ControllerApp/main.c`](../ControllerApp/main.c)):

```c
crypto_configure(config->e2e_key);                 // record passphrase, gate E2E
if (crypto_is_configured() && !crypto_generate_keypair())  // fresh X25519 pair
    platform_exit_with_error("Failed to generate end-to-end key pair\n");
```

The keypair exists *before* the first connection, so its public half is ready to
send during the handshake.

### Step 1 — client → server: public key as handshake init data
`connect_to_server` ([`Helpers/src/connect_to_server.c`](../Helpers/src/connect_to_server.c))
already sends a fixed set of init fields; the public key is appended as the
**last** field, for **every** one of the six connections:

```
[ uint32 max_buffer_size ]
[ uint32 client_type ]
[ byte   target ]            (read=0 / write=1)
[ byte   client_index ]     (controlled=0 / controller=1)
[ byte   notify_on_data_received ]
[ uint32 public_key_len ][ public_key_len bytes ]   <-- NEW (len=0 when E2E off)
```

This is plaintext — a public key is not secret. `network_send_public_key`
([`network.c`](../Platform/src/windows/network.c)) writes it; it sends a
zero-length blob when E2E is disabled so the wire layout stays uniform.

### Step 2 — server stores each key, then relays the *peer's* key
The relay reads the blob in `handleHandshake`
([`Server/ListenForClients/ListenForClients.go`](../Server/ListenForClients/ListenForClients.go))
using `SocketWrapper.ReadNBytes`, and stores it on `Client.PublicKey`
([`Server/Client/Client.go`](../Server/Client/Client.go)).

Once all six connections are in, `onAllConnected`
([`Server/ServerMain/ServerMain.go`](../Server/ServerMain/ServerMain.go)) sends
the all-connected signal and then calls `ControlChannel.SendPublicKeys`
([`Server/ControlChannel/ControlTunnel.go`](../Server/ControlChannel/ControlTunnel.go)),
which hands **each control channel its peer's** public key (index `0`↔`1`).
A client that offered no key (E2E off on its side) is skipped.

```
ALL_CLIENTS_CONNECTED  →  [ uint32 peer_key_len ][ peer_key_len bytes ]   (per control channel)
```

This happens **before** the data tunnels start, so the key is ready before any
data flows.

### Step 3 — client derives the session key
The control-channel reader `_thread_proc`
([`Helpers/include/control_chanel.c`](../Helpers/include/control_chanel.c)) reads
the peer key right after the `ALL_CLIENTS_CONNECTED` message and derives the key
**before** invoking the app callback that starts the screen/input threads:

```c
receive_one_byte(g_Socket, &msg);
if (msg == ALL_CLIENTS_CONNECTED_MSG && !key_exchanged) {
    if (!network_receive_peer_key(g_Socket))       // read peer key + derive
        platform_exit_with_error("End-to-end key exchange failed\n");
    key_exchanged = true;
}
callback();                                         // now data threads may seal/open
```

`network_receive_peer_key` reads the blob and calls `crypto_derive_session_key`.

### Sequence diagram

```
 Controlled                         Relay                          Controller
     |                                |                                |
     |--- connect (3x) + pub key A -->|                                |
     |                                |<-- connect (3x) + pub key B ---|
     |                                |  (waits for all 6 connections) |
     |                                |                                |
     |<------ ALL_CLIENTS_CONNECTED --|--- ALL_CLIENTS_CONNECTED ----->|
     |<------ peer key = B -----------|--- peer key = A -------------->|
     |                                |                                |
   derive K = HKDF(ECDH(a, B), ...)   |        derive K = HKDF(ECDH(b, A), ...)
     |                                |                                |
     |== AES-256-GCM screen frames ==>|== forwarded ciphertext =======>|  (open with K)
     |<= AES-256-GCM input frames ====|<= forwarded ciphertext ========|  (sealed with K)
```

Because ECDH is symmetric — `ECDH(a, B) == ECDH(b, A)` — both sides arrive at the
**same** secret and therefore the same session key `K`, without `K` ever
crossing the wire.

---

## 5. Deriving the session key

`crypto_derive_session_key(peer_pub, len)`:

1. **ECDH:** `shared = X25519(our_private_key, peer_public_key)` → 32-byte secret.
2. **HKDF-SHA256:**
   ```
   K = HKDF-SHA256(
         ikm  = shared,
         salt = "URemoteControl-e2e-v2",
         info = "URemoteControl-e2e-session-key-v2" || passphrase )
   ```
   produces the 32-byte (AES-256) session key.

The session key is then used by `crypto_seal` / `crypto_open` for every frame.

---

## 6. The passphrase now *authenticates* (it is not the key)

In the previous design the AES key was derived directly from the `e2e_key`
passphrase (PBKDF2). Now the key comes from ECDH, and the passphrase has a
different job:

- **It gates the feature.** Empty `e2e_key` ⇒ E2E off ⇒ no keypair, no handshake
  key field, plaintext frames (byte-for-byte the old behavior).
- **It authenticates the exchange.** The passphrase is mixed into the HKDF
  `info`. Two peers therefore derive the *same* `K` only if they share the same
  passphrase. This defeats an **active** relay that tries to man-in-the-middle
  the exchange by substituting its own public keys: it can complete two separate
  ECDH handshakes, but without the passphrase its derived keys won't match the
  clients', and the first AES-GCM frame fails to authenticate.

> Set the **same** `e2e_key` on both peers. A high-entropy value is recommended.
> An empty value means anonymous ECDH (safe against a *passive* relay only) — and
> in this build empty also means E2E is off.

---

## 7. The sealed frame wire format

Every encrypted payload (screen delta or input event) is sealed as:

```
[ uint32 length ][ 12-byte nonce ][ ciphertext (= plaintext length) ][ 16-byte GCM tag ]
```

The `uint32 length` prefix is exactly what the relay's data tunnel reads and
forwards, so the tunnel protocol is unchanged — the server forwards the opaque
`nonce|ciphertext|tag` blob without understanding it.

`send_encrypted_data` / `receive_encrypted_data`
([`network.c`](../Platform/src/windows/network.c)) wrap `crypto_seal` /
`crypto_open`. When the session key isn't established (E2E off) they fall back to
the original plaintext `send_data_with_size` / `receive_data` framing.

### What is encrypted
- **Screen frames** controlled → controller: a contiguous `[DeltaPacketInfo][zlib data]`
  built in [`ControlledApp/send_screen_buffer.c`](../ControlledApp/send_screen_buffer.c)
  and opened in [`ControllerApp/receive_screen_buffer_.c`](../ControllerApp/receive_screen_buffer_.c).
- **Input events** controller → controlled: `InputStruct` sealed in
  [`ControllerApp/send_io.c`](../ControllerApp/send_io.c) and opened in
  [`ControlledApp/receive_io.c`](../ControlledApp/receive_io.c).

The control channel only carries coordination bytes (and now the one-time peer
public key), not user data.

---

## 8. Proving it works (verification)

Each client writes a proof log next to its executable:
`bin/client_e2e_<pid>.log`. A healthy session shows:

- `status: ENABLED  key-exchange=X25519 ECDH  kdf=HKDF-SHA256  cipher=AES-256-GCM`
- `session key established ... key_id=XXXXXXXX` — **the `key_id` is identical on
  both peers**. `key_id` is the first 4 bytes of SHA-256 of the derived key;
  matching ids prove ECDH agreed on the same key purely from the exchanged
  public keys.
- `SELF-TEST decrypt round-trip: PASS` and `SELF-TEST tamper rejected by GCM auth: PASS`.
- `seal #N ...` on the sender and `open #N ... GCM auth OK` on the receiver.

On the server side, the log prints each connection with its `PublicKey:[..32 bytes..]`,
showing the key arriving as part of the handshake (index 0 and index 1 each show
a distinct, consistent key across their three connections).

To run the whole stack locally (safe debug mode, both clients on one machine):

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run-debug.ps1 -BuildFirst
# stop with:
powershell -ExecutionPolicy Bypass -File .\scripts\stop-debug.ps1
```

---

## 9. File map

| Area | File | Role |
|------|------|------|
| Crypto | `Platform/include/crypto.h`, `Platform/src/windows/crypto.c` | X25519 keypair, HKDF, AES-256-GCM seal/open, proof logging |
| Network | `Platform/include/network.h`, `Platform/src/windows/network.c` | `network_send_public_key`, `network_receive_peer_key`, encrypted data helpers |
| Handshake (client) | `Helpers/src/connect_to_server.c` | sends the public key as the last handshake field |
| Key derive (client) | `Helpers/include/control_chanel.c` | reads peer key after ALL_CLIENTS_CONNECTED, derives session key |
| Startup | `ControlledApp/main.c`, `ControllerApp/main.c` | `crypto_configure` + `crypto_generate_keypair` |
| Data paths | `ControlledApp/send_screen_buffer.c`, `ControllerApp/receive_screen_buffer_.c`, `ControllerApp/send_io.c`, `ControlledApp/receive_io.c` | seal/open screen & input |
| Server | `Server/Client/Client.go` | `PublicKey` field |
| Server | `Server/SocketWrapper/SocketWarraper.go` | `ReadNBytes` |
| Server | `Server/ListenForClients/ListenForClients.go` | reads the public-key handshake field |
| Server | `Server/ControlChannel/ControlTunnel.go` | `SendPublicKeys` relays each peer's key |
| Server | `Server/ServerMain/ServerMain.go` | calls `SendPublicKeys` at all-connected |
| TLS | `Server/generate_cert.bat`, `network.c` (`network_set_tls_config`) | optional transport TLS |

---

## 10. Security properties & limitations

**Properties**
- The relay sees only public keys and ciphertext; it cannot read screen or input.
- Forward secrecy from ephemeral X25519 keys.
- AES-256-GCM detects any tampering/corruption of frames.
- The pre-shared passphrase authenticates the exchange against an active MITM.

**Limitations / notes**
- Both peers must be configured with the **same** `e2e_key`; a mismatch causes a
  clean failure (GCM auth fails / key exchange errors), not silent insecurity.
- An **empty** passphrase disables E2E entirely (plaintext) — by design, for
  debugging and wire-compatibility.
- The passphrase guards against an active MITM; treat it as a real secret.
- Nonces are random 96-bit per frame, which is safe for these message volumes;
  there is no long-lived counter to overflow.
