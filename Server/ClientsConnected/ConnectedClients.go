package ClientsConnected

import (
	"Server/Client"
	"Server/ControlChannel"
	"Server/ListenForClients"
	"Server/Tunnel"
	"container/list"
	"fmt"
)

type ClientsPair struct {
	clients map[Client.Index]*Client.Client
}

type AllTunnels struct {
	Tunnels *list.List
}

func createClientsPair() *ClientsPair {
	out := new(ClientsPair)

	out.clients = make(map[Client.Index]*Client.Client)

	return out
}

var clientsPairs = map[Client.Type]*ClientsPair{
	Client.ControlChannelClientType: createClientsPair(),

	Client.SyncScreenBufferClientType: createClientsPair(),
	Client.SyncIOClientType:           createClientsPair(),
}

var numOfPlacesLeft = len(clientsPairs) * 2

func TryAddClient(client *Client.Client) ListenForClients.TryAddClientRes {

	out := ListenForClients.TryAddClientRes{
		CanAddClient:      false,
		CloseListenSocket: false,
	}

	if clientsPairs[client.ClientType].clients[client.Index] == nil {
		if client.ClientType == Client.ControlChannelClientType {
			ControlChannel.AddControlChannel(
				client,
				client.Index)
		}

		fmt.Printf("New client connected %+v\n", client)

		clientsPairs[client.ClientType].clients[client.Index] = client

		out.CanAddClient = true

		numOfPlacesLeft--
		if numOfPlacesLeft == 0 {
			out.CloseListenSocket = true
		}

		return out
	}

	return out
}

func ConstructAllTunnels() *AllTunnels {
	out := new(AllTunnels)
	out.Tunnels = list.New()

	for key, value := range clientsPairs {
		if key == Client.ControlChannelClientType {
			continue
		}

		var clientTargetRead *Client.Client = nil
		var clientTargetWrite *Client.Client = nil

		if value.clients[0].DataTarget == Client.TargetRead {
			clientTargetRead = value.clients[0]
			clientTargetWrite = value.clients[1]
		} else {
			clientTargetRead = value.clients[1]
			clientTargetWrite = value.clients[0]
		}

		out.Tunnels.PushBack(Tunnel.CreateTunnel(
			clientTargetWrite,
			clientTargetRead))
	}

	return out
}
