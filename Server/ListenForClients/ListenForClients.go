package ListenForClients

import (
	"Server/Client"
	"Server/SocketWrapper"
	"fmt"
	"log"
	"net"
	"os"
	"sync"
	"time"
)

const connectionType = "tcp"
const defaultHost = "0.0.0.0"
const defaultPort = "80"
const serverPortEnv = "UREMOTE_SERVER_PORT"
const listenHostEnv = "UREMOTE_LISTEN_HOST"

const NoClientsConnectedTimeout = 10

// Upper bound on the end-to-end public-key blob in the handshake (X25519 keys
// are 32 bytes; this guards against a malformed length triggering a huge alloc).
const maxPublicKeyLen = 1024

func resolvePort() string {
	if len(os.Args) > 1 && os.Args[1] != "" {
		return os.Args[1]
	}

	if envPort := os.Getenv(serverPortEnv); envPort != "" {
		return envPort
	}

	return defaultPort
}

// resolveHost picks the interface to listen on. Defaults to 0.0.0.0 (all
// interfaces, for LAN use). Set UREMOTE_LISTEN_HOST=127.0.0.1 to listen on
// loopback only: loopback traffic is exempt from the Windows Firewall, so this
// avoids the "allow this app to communicate on networks" prompt when running
// everything locally.
func resolveHost() string {
	if h := os.Getenv(listenHostEnv); h != "" {
		return h
	}
	return defaultHost
}

type TryAddClientRes struct {
	CanAddClient      bool
	CloseListenSocket bool
}

type TryAddClientCallback func(
	client *Client.Client) TryAddClientRes

type OnAllClientsConnectedCallback func()

func StartListenForClients(
	callBack TryAddClientCallback,
	onAllConnectedCallback OnAllClientsConnectedCallback) {
	port := resolvePort()

	clientsConnected := false
	mutex := sync.Mutex{}

	host := resolveHost()

	fmt.Println("Start listening on " + host + ":" + port)

	go func() {
		time.Sleep(NoClientsConnectedTimeout * time.Second)

		mutex.Lock()
		if !clientsConnected {
			os.Exit(0)
		}
		mutex.Unlock()
	}()

	listener, _error := net.Listen(connectionType, host+":"+port)

	if _error != nil {
		panic("Failed to create listen socket, error msg" + _error.Error())
	}

	for {
		newSocket, _error := listener.Accept()

		if _error != nil {
			panic("Failed to accept client, error msg" + _error.Error())
		}

		mutex.Lock()
		clientsConnected = true
		mutex.Unlock()

		// A malformed or probe connection (e.g. a port check that opens and
		// immediately closes the socket) must not take down the whole server,
		// so the handshake is handled in isolation and any panic is recovered.
		res, ok := handleHandshake(newSocket, callBack)
		if !ok {
			continue
		}

		if res.CloseListenSocket {
			_error := listener.Close()
			if _error != nil {
				panic("Failed to close listen socket, error msg" + _error.Error())
			}
			onAllConnectedCallback()
			return
		}

	}

}

// handleHandshake reads a client's handshake and registers it. It returns
// ok == false (after closing the socket) if the connection misbehaves, so the
// accept loop can simply move on to the next connection.
func handleHandshake(
	conn net.Conn,
	callBack TryAddClientCallback) (res TryAddClientRes, ok bool) {

	defer func() {
		if r := recover(); r != nil {
			fmt.Println("Dropping connection during handshake:", r)
			_ = conn.Close()
			ok = false
		}
	}()

	socketWrapper := SocketWrapper.CreateSocketWrapper(conn)

	maxBufferSize := socketWrapper.ReadUin32()
	clientType := socketWrapper.ReadUin32()
	target := socketWrapper.ReadOneByte()
	clientIndex := socketWrapper.ReadOneByte()
	notifyOnDataReceivedByte := socketWrapper.ReadOneByte()

	notifyOnDataReceived := notifyOnDataReceivedByte == 1

	// End-to-end public key, sent as the last handshake field (a zero-length
	// blob when E2E is off). The relay only forwards it to the peer; it never
	// uses it, so it cannot decrypt the tunnelled data.
	publicKeyLen := socketWrapper.ReadUin32()
	var publicKey []byte
	if publicKeyLen > 0 {
		if publicKeyLen > maxPublicKeyLen {
			panic("Public key blob too large during handshake")
		}
		publicKey = socketWrapper.ReadNBytes(publicKeyLen)
	}

	// E2E proof: log the public key the relay received from each connection
	// (clientType 0=control 1=controller-data 2=controlled-data). A length of 0
	// means that side has E2E disabled, which will break the peer's exchange.
	log.Printf("[E2E-RELAY] handshake: clientType=%d index=%d publicKeyLen=%d", clientType, clientIndex, publicKeyLen)

	client := new(Client.Client)

	client.MaxBufferSize = maxBufferSize
	client.ClientType = Client.Type(clientType)
	client.DataTarget = Client.DataTarget(target)
	client.Index = Client.Index(clientIndex)
	client.NotifyOnDataReceived = notifyOnDataReceived
	client.PublicKey = publicKey
	client.SocketWrapper = socketWrapper

	res = callBack(client)

	canAddClientResBuff := byte(0)
	if res.CanAddClient {
		canAddClientResBuff = byte(1)
	} else {
		fmt.Println("Failed to connect client to server")
	}

	socketWrapper.SendOneByte(canAddClientResBuff)

	ok = true
	return
}
