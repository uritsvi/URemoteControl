package ListenForClients

import (
	"Server/Client"
	"Server/SocketWrapper"
	"fmt"
	"net"
	"os"
	"sync"
	"time"
)

const connectionType = "tcp"
const host = "0.0.0.0"

const NoClientsConnectedTimeout = 10

var port = os.Args[1]

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

	clientsConnected := false
	mutex := sync.Mutex{}

	fmt.Println("Start listening on port " + port)

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

		mutex.Lock()
		clientsConnected = true
		mutex.Unlock()

		if _error != nil {
			panic("Failed to accept client, error msg" + _error.Error())
		}

		socketWrapper := SocketWrapper.CreateSocketWrapper(newSocket)

		maxBufferSize := socketWrapper.ReadUin32()
		clientType := socketWrapper.ReadUin32()
		target := socketWrapper.ReadOneByte()
		clientIndex := socketWrapper.ReadOneByte()
		notifyOnDataReceivedByte := socketWrapper.ReadOneByte()

		notifyOnDataReceived := false

		if notifyOnDataReceivedByte == 1 {
			notifyOnDataReceived = true
		}

		client := new(Client.Client)

		client.MaxBufferSize = maxBufferSize
		client.ClientType = Client.Type(clientType)
		client.DataTarget = Client.DataTarget(target)
		client.Index = Client.Index(clientIndex)
		client.NotifyOnDataReceived = notifyOnDataReceived
		client.SocketWrapper = socketWrapper

		res := callBack(
			client)

		canAddClientResBuff := byte(0)
		if res.CanAddClient {
			canAddClientResBuff = byte(1)
		} else {
			fmt.Println("Failed to connect client to server")
		}

		socketWrapper.SendOneByte(canAddClientResBuff)

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
