package main

import (
	"log"
	"fmt"
	"net/rpc"
	"github.com/sideshow/apns2"
	"github.com/sideshow/apns2/certificate"
	"../shared-server"
	"net"
	"errors"
	"encoding/gob"
	"os"
)

func registerArith(server *rpc.Server, arith shared_server.Arith, pushService shared_server.PushService) {
	// registers Arith interface by name of `Arithmetic`.
	// If you want this name to be same as the type name, you
	// can use server.Register instead.
	server.RegisterName("Arithmetic", arith)
	server.RegisterName("PushService", pushService)
}

type Arith int

func (t *Arith) Multiply(args *shared_server.Args, reply *int) error {
	*reply = args.A * args.B
	return nil
}

func (t *Arith) Divide(args *shared_server.Args, quo *shared_server.Quotient) error {
	if args.B == 0 {
		return errors.New("divide by zero")
	}
	quo.Quo = args.A / args.B
	quo.Rem = args.A % args.B
	return nil
}

type PushServiceData struct {
	PushTokenMap map[string][16]byte // Push Token --> User ID
	UserIdMap map[[16]byte]string // User ID --> Push Token
}

func (t *PushServiceData) Add(id []byte, pushToken string) {
	var idFixed [16]byte
	copy(idFixed[:], id)
	t.PushTokenMap[pushToken] = idFixed
	t.UserIdMap[idFixed] = pushToken
}

type PushService struct {
	data PushServiceData
}

func (t *PushService) RegisterPushToken(args *shared_server.PushToken, reply *int) error {
	log.Printf("Register push token: %v, %v", args.Domain, args.PushToken)
	t.data.Add(args.UserId, args.PushToken)
	log.Printf("pushTokenMap len: %v", len(t.data.PushTokenMap))
	pushKeyDbFile, err := os.Create("db/pushkeydb")
	if err != nil {
		log.Fatalf("pushkeydb create failed: %v", err.Error())
	}
	encoder := gob.NewEncoder(pushKeyDbFile)
	encoder.Encode(t.data)
	pushKeyDbFile.Close()
	*reply = 1
	return nil
}

func main() {
	// Set default log format
	log.SetFlags(log.Lshortfile | log.LstdFlags)
	log.Println("Greetings from push server")
	// Create db directory to save user database
	os.MkdirAll("db", os.ModePerm)
	//Creating an instance of struct which implement Arith interface
	arith := new(Arith)
	pushService := &PushService{
		PushServiceData{
			make(map[string][16]byte),
			make(map[[16]byte]string),
		},
	}
	pushKeyDbFile, err := os.Open("db/pushkeydb")
	if err != nil {
		if os.IsNotExist(err) {
			log.Printf("Push key DB not found.")
		} else {
			log.Fatalf("pushkeydb create failed: %v", err.Error())
		}
	} else {
		defer pushKeyDbFile.Close()
		decoder := gob.NewDecoder(pushKeyDbFile)
		err := decoder.Decode(&pushService.data)
		if err != nil {
			log.Fatalf("pushkeydb decode failed: %v", err.Error())
		}
		log.Printf("pushTokenMap loaded from file. len: %v", len(pushService.data.PushTokenMap))
	}

	//rpc.Register(arith)

	// Register a new rpc server (In most cases, you will use default server only)
	// And register struct we created above by name "Arith"
	// The wrapper method here ensures that only structs which implement Arith interface
	// are allowed to register themselves.
	server := rpc.NewServer()
	registerArith(server, arith, pushService)

	// Listen for incoming tcp packets on specified port.
	l, e := net.Listen("tcp", ":20171")
	if e != nil {
		log.Fatal("listen error:", e)
	}

	// This statement links rpc server to the socket, and allows rpc server to accept
	// rpc request coming from that socket.
	server.Accept(l)
}

func post_test_push() {
	cert, err := certificate.FromP12File("cert/dev/cert.p12", "")
	if err != nil {
		log.Fatal("Cert Error:", err)
	}

	notification := &apns2.Notification{}
	notification.DeviceToken = "fb719d7a7a1629c3c5bda507e5c2940d3cf5bca8a0270508faf97e99624752fd"
	notification.Topic = "com.popsongremix.laidoff"
	notification.Payload = []byte(`{"aps":{"alert":"어서와, 리모트 푸시는 처음이지?"}}`)

	client := apns2.NewClient(cert).Development()
	res, err := client.Push(notification)

	if err != nil {
		log.Fatal("Error:", err)
	}

	fmt.Printf("%v %v %v\n", res.StatusCode, res.ApnsID, res.Reason)
}
