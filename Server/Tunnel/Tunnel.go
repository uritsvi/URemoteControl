package Tunnel

import (
	"Server/Client"
	"Server/ControlChannel"
	"fmt"
	"os"
	"sync"
)

type Tunnel struct {
	send    *Client.Client
	receive *Client.Client
}

type tunnelRunParams struct {
	bufferChan      chan *[]byte
	fullBuffersChan chan *[]byte
	sizeChan        chan uint32
	buffersPool     sync.Pool
}

func CreateTunnel(send *Client.Client,
	receive *Client.Client) *Tunnel {

	out := new(Tunnel)

	out.send = send
	out.receive = receive

	return out
}

func (tunnel *Tunnel) RunTunnel() {

	tunnelParams := new(tunnelRunParams)

	tunnelParams.bufferChan = make(chan *[]byte)
	tunnelParams.sizeChan = make(chan uint32)
	tunnelParams.fullBuffersChan = make(chan *[]byte)
	tunnelParams.buffersPool = sync.Pool{New: func() interface{} {
		return make([]byte, tunnel.send.MaxBufferSize)
	}}

	go read(tunnelParams, tunnel.send)
	go write(tunnelParams, tunnel.receive)
}

func read(params *tunnelRunParams,
	client *Client.Client) {

	// A client closing its connection (window closed, app exited, network
	// drop) surfaces as a read error from the socket wrapper. Treat it as a
	// clean end of session instead of crashing the server with a stack trace.
	defer func() {
		if r := recover(); r != nil {
			fmt.Println("A client disconnected; ending session.")
			os.Exit(0)
		}
	}()

	for {
		size := client.SocketWrapper.ReadUin32()

		params.sizeChan <- size

		var i uint32 = 0

		for i < size {
			buffer := params.buffersPool.Get().([]byte)
			if size-i < client.MaxBufferSize {
				buffer = buffer[0 : size-i]
			}

			n := client.SocketWrapper.ReadBuffer(&buffer)

			buff := buffer[0:n]

			params.bufferChan <- &buff
			params.fullBuffersChan <- &buffer
			i += n
		}

		if client.NotifyOnDataReceived {
			ControlChannel.SendMessageToOneClient(
				client.Index,
				ControlChannel.DataReceived)

		}
	}

}

func write(params *tunnelRunParams,
	client *Client.Client) {

	defer func() {
		if r := recover(); r != nil {
			fmt.Println("A client disconnected; ending session.")
			os.Exit(0)
		}
	}()

	for {
		size := <-params.sizeChan
		client.SocketWrapper.SendUint32(size)
		var i uint32 = 0
		for i < size {
			buffer := <-params.bufferChan
			client.SocketWrapper.SendFullBuffer(buffer)
			i += uint32(len(*buffer))

			fullBuffers := <-params.fullBuffersChan
			params.buffersPool.Put(*fullBuffers)
		}

	}
}
