package ControlChannel

import "Server/Client"

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
