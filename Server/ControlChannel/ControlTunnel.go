package ControlChannel

import (
	"Server/Client"
	"log"
)

var controlTunnels map[Client.Index]*Client.Client = make(map[Client.Index]*Client.Client)

func AddControlChannel(
	client *Client.Client,
	clientIndex Client.Index) {
	controlTunnels[clientIndex] = client
}

func SendMessageToAll(msg byte) {
	for _, value := range controlTunnels {
		value.SocketWrapper.SendOneByte(msg)
	}
}

func SendMessageToOneClient(
	index Client.Index,
	msg byte) {
	controlChannel := controlTunnels[index]
	controlChannel.SocketWrapper.SendOneByte(msg)
}

// SendPublicKeys completes the end-to-end key exchange: it hands each client its
// peer's public key (collected during the connection handshake) over the
// control channel as a length-prefixed blob. A client that offered no key (E2E
// off on its side) is skipped, so its control stream stays a pure message
// stream. Public keys are not secret, so relaying them keeps E2E intact.
func SendPublicKeys() {
	log.Printf("[E2E-RELAY] SendPublicKeys: %d control channel(s) registered", len(controlTunnels))
	for index, client := range controlTunnels {
		if len(client.PublicKey) == 0 {
			// This client did not enable E2E; do not send it a key blob.
			log.Printf("[E2E-RELAY] index=%d offered NO public key (E2E off on its side) -> skipping; its peer will receive no key and FAIL the exchange", index)
			continue
		}

		peer := controlTunnels[1-index]

		var peerKey []byte
		if peer != nil {
			peerKey = peer.PublicKey
		}

		if len(peerKey) == 0 {
			log.Printf("[E2E-RELAY] index=%d: peer(index=%d) key is EMPTY -> forwarding len=0 (this client's key exchange will FAIL)", index, 1-index)
		} else {
			first := peerKey
			if len(first) > 8 {
				first = first[:8]
			}
			log.Printf("[E2E-RELAY] index=%d: forwarding peer(index=%d) public key, len=%d, first bytes=%v", index, 1-index, len(peerKey), first)
		}

		client.SocketWrapper.SendUint32(uint32(len(peerKey)))
		if len(peerKey) > 0 {
			client.SocketWrapper.SendFullBuffer(&peerKey)
		}
	}
	log.Println("[E2E-RELAY] SendPublicKeys: done (peer keys delivered)")
}
