package Client

import "Server/SocketWrapper"

type DataTarget byte
type Index byte
type Type uint32
type TunnelIndex uint32

const TargetRead DataTarget = 0
const TargetWrite DataTarget = 1

type Client struct {
	MaxBufferSize        uint32
	ClientType           Type
	DataTarget           DataTarget
	Index                Index
	NotifyOnDataReceived bool
	// PublicKey is the client's end-to-end X25519 public key, sent as part of
	// the connection handshake. Empty when end-to-end encryption is disabled.
	// The relay forwards each client its peer's key but never uses it itself.
	PublicKey     []byte
	SocketWrapper *SocketWrapper.SocketWrapper
}
