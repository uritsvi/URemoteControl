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
	SocketWrapper        *SocketWrapper.SocketWrapper
}
