package ServerMain

import (
	"Server/Client"
	"Server/ClientsConnected"
	"Server/ControlChannel"
	"Server/ListenForClients"
	"Server/Tunnel"
	"fmt"
	"io"
	"log"
	"os"
	"sync"
	"time"
)

// setupLogging mirrors log output to both the console and a per-port file
// (relay_<port>.log in the relay's working directory) so the end-to-end
// key-exchange proof can be read even though the relay runs in its own console.
func setupLogging() {
	port := "unknown"
	if len(os.Args) > 1 && os.Args[1] != "" {
		port = os.Args[1]
	}
	f, err := os.OpenFile("relay_"+port+".log", os.O_CREATE|os.O_APPEND|os.O_WRONLY, 0644)
	if err != nil {
		log.Println("[E2E-RELAY] could not open relay log file:", err)
		return
	}
	log.SetOutput(io.MultiWriter(os.Stdout, f))
	log.SetFlags(log.LstdFlags | log.Lmicroseconds)
	log.Printf("[E2E-RELAY] logging started for relay on port %s", port)
}

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
	log.Println("[E2E-RELAY] all clients connected; beginning key exchange")

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
	setupLogging()
	fmt.Println("Server started successfully")

	go ListenForClients.StartListenForClients(
		onNewConnected,
		onAllConnected)

	for {
		time.Sleep(time.Second)
	}
}
