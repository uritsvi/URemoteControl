package SocketWrapper

import (
	"encoding/binary"
	"errors"
	"io"
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
	// io.ReadFull guarantees the whole buffer is read; a bare Read can return
	// fewer bytes than requested when TCP splits the data across segments.
	_, _error := io.ReadFull(socket.socket, buffer)

	if _error != nil {
		panic("Failed to receive data from client, error msg" + _error.Error())
	}

	return buffer[0]
}

func (socket SocketWrapper) ReadUin32() (out uint32) {
	buffer := make([]byte, 4)
	// io.ReadFull is essential here: a length prefix split across TCP segments
	// would otherwise be parsed from a partial buffer, desyncing the stream.
	_, _error := io.ReadFull(socket.socket, buffer)

	if _error != nil {
		panic("Failed to receive data from client, error msg" + _error.Error())
	}

	out = binary.LittleEndian.Uint32(buffer)

	return out
}

func (socket SocketWrapper) ReadNBytes(n uint32) []byte {
	buffer := make([]byte, n)
	// io.ReadFull guarantees the whole blob is read even if TCP splits it.
	_, _error := io.ReadFull(socket.socket, buffer)

	if _error != nil {
		panic("Failed to receive data from client, error msg" + _error.Error())
	}

	return buffer
}

func (socket SocketWrapper) ReadBuffer(buffer *[]byte) uint32 {

	n, _error := socket.socket.Read(*buffer)

	if _error != nil {
		panic("Failed to receive data from socket, error msg" + _error.Error())
	}

	return uint32(n)
}

func (socket SocketWrapper) IsConnected() bool {
	err := socket.socket.SetReadDeadline(time.Now().Add(time.Second))
	if err != nil {
		return false
	}

	buffer := make([]byte, 1)
	_, err = socket.socket.Read(buffer)
	if err == nil {
		return true
	}

	var netErr net.Error
	if errors.As(err, &netErr) && netErr.Timeout() {
		return true
	}

	return false
}
