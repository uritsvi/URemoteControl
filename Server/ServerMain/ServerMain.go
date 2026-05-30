package ServerMain

import (
	"Server/Client"
	"Server/ClientsConnected"
	"Server/ControlChannel"
	"Server/ListenForClients"
	"Server/Tunnel"
	"fmt"
	"os"
	"sync"
	"time"
)

var lock = new(sync.Mutex)
var allConnected = false

func checkIfStillConnected(client *Client.Client) {
	for !allConnected {
		lock.Lock()
		if client.SocketWrapper.IsConnected() {
			lock.Unlock()
			time.Sleep(5 * time.Second)

			continue
		}

		os.Exit(0)
	}
}

func onAllConnected() {
	lock.Lock()

	fmt.Println("All clients are connected")

	ControlChannel.SendMessageToAll(ControlChannel.AllClientsConnectedMsg)

	// Deliver each client its peer's end-to-end public key over the control
	// channel, right after the all-connected signal and before the tunnels
	// start, so the session key is established before any data flows.
	ControlChannel.SendPublicKeys()

	allTunnels := ClientsConnected.ConstructAllTunnels()
	for tunnelElement := allTunnels.Tunnels.Front(); tunnelElement != nil; tunnelElement = tunnelElement.Next() {
		tunnelElement.Value.(*Tunnel.Tunnel).RunTunnel()
	}

	allConnected = true

	lock.Unlock()
}

func onNewConnected(client *Client.Client) ListenForClients.TryAddClientRes {
	res := ClientsConnected.TryAddClient(client)

	if !res.CanAddClient {
		return res
	}

	if client.DataTarget == Client.TargetRead {
		go checkIfStillConnected(client)
	}

	return res
}

func ServerMain() {
	fmt.Println("Server started successfully")

	go ListenForClients.StartListenForClients(
		onNewConnected,
		onAllConnected)

	for {
		time.Sleep(time.Second)
	}
}
