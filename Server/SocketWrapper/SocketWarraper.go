package SocketWrapper

import (
	"encoding/binary"
	"net"
	"time"
)

type SocketWrapper struct {
	socket net.Conn
}

func CreateSocketWrapper(socket net.Conn) *SocketWrapper {
	out := new(SocketWrapper)

	out.socket = socket

	return out
}

func (socket SocketWrapper) SendOneByte(data byte) {
	_, _error := socket.socket.Write([]byte{data})

	if _error != nil {
		panic("Failed to send data to client, error msg " + _error.Error())
	}
}
func (socket SocketWrapper) SendUint32(i uint32) {
	buffer := make([]byte, 4)
	binary.LittleEndian.PutUint32(buffer, i)

	_, _error := socket.socket.Write(buffer)
	if _error != nil {
		panic("Failed to send uint32, error msg" + _error.Error())
	}
}

func (socket SocketWrapper) SendFullBuffer(buffer *[]byte) {

	i := 0
	size := len(*buffer)
	buff := *buffer

	for i < size {
		n, _error := socket.socket.Write(buff[i:size])

		if _error != nil {
			panic("Failed to send data, error msg" + _error.Error())
		}

		i += n
	}

}

func (socket SocketWrapper) ReadOneByte() byte {
	buffer := make([]byte, 1)
	_, _error := socket.socket.Read(buffer)

	if _error != nil {
		panic("Failed to receive data from client, error msg" + _error.Error())
	}

	return buffer[0]
}

func (socket SocketWrapper) ReadUin32() (out uint32) {
	buffer := make([]byte, 4)
	_, _error := socket.socket.Read(buffer)

	if _error != nil {
		panic("Failed to receive data from client, error msg" + _error.Error())
	}

	out = binary.LittleEndian.Uint32(buffer)

	return out
}

func (socket SocketWrapper) ReadBuffer(buffer *[]byte) uint32 {

	n, _error := socket.socket.Read(*buffer)

	if _error != nil {
		panic("Failed to receive data from socket, error msg" + _error.Error())
	}

	return uint32(n)
}

func (socket SocketWrapper) IsConnected() bool {
	_error := socket.socket.SetReadDeadline(time.Now().Add(time.Second))
	if _error != nil {
		panic("Failed to set conn read data time")
	}

	buffer := make([]byte, 1)
	_, _error = socket.socket.Read(buffer)
	if _error == nil || _error.(net.Error).Timeout() {
		return true

	}
	return false
}
