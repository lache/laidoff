package main

import (
	"log"
	"net"
	"encoding/binary"
	"bytes"
	"unsafe"
	"os"
	"encoding/json"
	"./nickdb"
	"./convert"
	"./user"
	"./service"
	"./handler"
	"./config"
	"time"
	mathrand "math/rand"
)

// #include "../src/puckgamepacket.h"
import "C"

func main() {
	// Set default log format
	log.SetFlags(log.Lshortfile | log.LstdFlags)
	log.Println("Greetings from match server")
	// Create db directory to save user database
	os.MkdirAll("db", os.ModePerm)
	// Seed a new random number
	mathrand.Seed(time.Now().Unix())
	// Load nick name database
	nickDb := Nickdb.LoadNickDb()
	// Service List
	serviceList := service.NewServiceList()
	log.Printf("Sample nick: %v", Nickdb.PickRandomNick(&nickDb))
	// Load conf.json
	confFile, err := os.Open("conf.json")
	if err != nil {
		log.Fatalf("conf.json open error:%v", err.Error())
	}
	confFileDecoder := json.NewDecoder(confFile)
	conf := config.ServerConfig{}
	err = confFileDecoder.Decode(&conf)
	if err != nil {
		log.Fatalf("conf.json parse error:%v", err.Error())
	}
	// Test RPC
	//testRpc(serviceList)
	// Create 1 vs. 1 match queue
	matchQueue := make(chan user.UserAgent)
	// Start match worker goroutine
	go matchWorker(conf, matchQueue)
	// Open TCP service port and listen for game clients
	l, err := net.Listen(conf.ConnType, conf.ConnHost+":"+conf.ConnPort)
	if err != nil {
		log.Fatalln("Error listening:", err.Error())
	}
	defer l.Close()
	log.Printf("Listening %v for match service...", conf.ConnHost+":"+conf.ConnPort)
	for {
		conn, err := l.Accept()
		if err != nil {
			log.Println("Error accepting: ", err.Error())
		} else {
			go handleRequest(conf, &nickDb, conn, matchQueue, serviceList)
		}
	}
}

func testRpc(serviceList *service.ServiceList) {
	log.Println(serviceList.Arith.Multiply(5, 6))
	log.Println(serviceList.Arith.Divide(500, 10))
	log.Println(serviceList.Arith.RegisterPushToken(300*time.Millisecond, user.UserId{1, 2, 3, 4}, 500, "test-push-token"))
	log.Println(serviceList.Rank.Set(300*time.Millisecond, user.UserId{1}, 100, "TestUser1"))
	log.Println(serviceList.Rank.Set(300*time.Millisecond, user.UserId{2}, 200, "TestUser2"))
	log.Println(serviceList.Rank.Set(300*time.Millisecond, user.UserId{3}, 300, "TestUser3"))
	log.Println(serviceList.Rank.Set(300*time.Millisecond, user.UserId{4}, 50, "TestUser4"))
	log.Println(serviceList.Rank.Set(300*time.Millisecond, user.UserId{5}, 20, "TestUser5"))
	log.Println(serviceList.Rank.Set(300*time.Millisecond, user.UserId{6}, 20, "TestUser6"))
	log.Println(serviceList.Rank.Set(300*time.Millisecond, user.UserId{7}, 20, "TestUser7"))
	log.Println(serviceList.Rank.Set(300*time.Millisecond, user.UserId{8}, 10, "한글닉넴8"))
	log.Println(serviceList.Rank.GetLeaderboard(300*time.Millisecond, 0, 20))
	log.Println(serviceList.Rank.Get(300*time.Millisecond, user.UserId{5}))
}

func createBattleInstance(conf config.ServerConfig, c1 user.UserAgent, c2 user.UserAgent) {
	// Connect to battle service
	tcpAddr, err := net.ResolveTCPAddr(conf.BattleServiceConnType, conf.BattleServiceHost+":"+conf.BattleServicePort)
	if err != nil {
		log.Fatalf("ResolveTCPAddr error! - %v", err.Error())
	}
	conn, err := net.DialTCP("tcp", nil, tcpAddr)
	if err != nil {
		log.Fatalf("DialTCP error! - %v", err.Error())
	}
	// Send create battle request

	createBattleBuf := convert.Packet2Buf(convert.NewCreateBattle(
		c1.Db.Id,
		c2.Db.Id,
		c1.Db.Nickname,
		c2.Db.Nickname,
		))
	_, err = conn.Write(createBattleBuf)
	if err != nil {
		log.Fatalf("Send LSBPT_LWSPHEREBATTLEPACKETCREATEBATTLE failed")
	}
	// Wait for a reply
	createBattleOk := C.LWPCREATEBATTLEOK{}
	createBattleOkBuf := make([]byte, unsafe.Sizeof(C.LWPCREATEBATTLEOK{}))
	createBattleOkBufLen, err := conn.Read(createBattleOkBuf)
	if err != nil {
		log.Printf("Recv LSBPT_LWSPHEREBATTLEPACKETCREATEBATTLE reply failed")
		return
	}
	if createBattleOkBufLen != int(unsafe.Sizeof(C.LWPCREATEBATTLEOK{})) {
		log.Printf("Recv LSBPT_LWSPHEREBATTLEPACKETCREATEBATTLE reply size error")
		return
	}
	createBattleOkBufReader := bytes.NewReader(createBattleOkBuf)
	err = binary.Read(createBattleOkBufReader, binary.LittleEndian, &createBattleOk)
	if err != nil {
		log.Printf("binary.Read fail")
		return
	}
	if createBattleOk.Size == C.ushort(unsafe.Sizeof(C.LWPCREATEBATTLEOK{})) && createBattleOk.Type == C.LPGP_LWPCREATEBATTLEOK {
		// No error! so far ... proceed battle
		log.Printf("MATCH %v and %v matched successfully!", c1.Conn.RemoteAddr(), c2.Conn.RemoteAddr())
		c1.Conn.Write(createMatched2Buf(conf, createBattleOk, createBattleOk.C1_token, 1, c2.Db.Nickname))
		c2.Conn.Write(createMatched2Buf(conf, createBattleOk, createBattleOk.C2_token, 2, c1.Db.Nickname))
	} else {
		log.Printf("Recv LSBPT_LWSPHEREBATTLEPACKETCREATEBATTLE reply corrupted")
	}
}

func createMatched2Buf(conf config.ServerConfig, createBattleOk C.LWPCREATEBATTLEOK, token C.uint, playerNo C.int, targetNickname string) []byte {
	publicAddr, err := net.ResolveTCPAddr(conf.BattlePublicServiceConnType, conf.BattlePublicServiceHost+":"+conf.BattlePublicServicePort)
	if err != nil {
		log.Panicf("BattlePublicService conf parse error: %v", err.Error())
	}
	publicAddrIpv4 := publicAddr.IP.To4()

	return convert.Packet2Buf(convert.NewMatched2(
		publicAddr.Port,
		publicAddrIpv4,
		int(createBattleOk.Battle_id),
		uint(token),
		int(playerNo),
		targetNickname,
	))
}

func matchWorker(conf config.ServerConfig, matchQueue <-chan user.UserAgent) {
	for {
		c1 := <-matchQueue
		c2 := <-matchQueue
		if c1.Conn == c2.Conn {
			sendRetryQueue(c1.Conn)
		} else {
			log.Printf("%v and %v matched! (maybe)", c1.Conn.RemoteAddr(), c2.Conn.RemoteAddr())
			maybeMatchedBuf := convert.Packet2Buf(&C.LWPMAYBEMATCHED{
				C.ushort(unsafe.Sizeof(C.LWPMAYBEMATCHED{})),
				C.LPGP_LWPMAYBEMATCHED,
			})
			n1, err1 := c1.Conn.Write(maybeMatchedBuf)
			n2, err2 := c2.Conn.Write(maybeMatchedBuf)
			if n1 == 4 && n2 == 4 && err1 == nil && err2 == nil {
				go createBattleInstance(conf, c1, c2)
			} else {
				// Match cannot be proceeded
				checkMatchError(err1, c1.Conn)
				checkMatchError(err2, c2.Conn)
			}
		}
	}
}

func checkMatchError(err error, conn net.Conn) {
	if err != nil {
		log.Printf("%v: %v error!", conn.RemoteAddr(), err.Error())
	} else {
		sendRetryQueue(conn)
	}
}
func sendRetryQueue(conn net.Conn) {
	retryQueueBuf := convert.Packet2Buf(&C.LWPRETRYQUEUE{
		C.ushort(unsafe.Sizeof(C.LWPRETRYQUEUE{})),
		C.LPGP_LWPRETRYQUEUE,
	})
	_, retrySendErr := conn.Write(retryQueueBuf)
	if retrySendErr != nil {
		log.Printf("%v: %v error!", conn.RemoteAddr(), retrySendErr.Error())
	} else {
		log.Printf("%v: Send retry match packet to client", conn.RemoteAddr())
	}
}

func handleRequest(conf config.ServerConfig, nickDb *Nickdb.NickDb, conn net.Conn, matchQueue chan<- user.UserAgent, serviceList *service.ServiceList) {
	log.Printf("Accepting from %v", conn.RemoteAddr())
	for {
		buf := make([]byte, 1024)
		//conn.SetReadDeadline(time.Now().Add(10000 * time.Millisecond))
		readLen, err := conn.Read(buf)
		if readLen == 0 {
			log.Printf("%v: readLen is zero.", conn.RemoteAddr())
			break
		}
		if err != nil {
			log.Printf("%v: Error reading: %v", conn.RemoteAddr(), err.Error())
		}
		log.Printf("%v: Packet received (readLen=%v)", conn.RemoteAddr(), readLen)
		packetSize := binary.LittleEndian.Uint16(buf)
		packetType := binary.LittleEndian.Uint16(buf[2:])
		log.Printf("  Size %v", packetSize)
		log.Printf("  Type %v", packetType)
		switch packetType {
		case C.LPGP_LWPQUEUE2:
			handler.HandleQueue2(matchQueue, buf, conn)
		case C.LPGP_LWPSUDDENDEATH:
			handler.HandleSuddenDeath(conf, buf) // relay 'buf' to battle service
		case C.LPGP_LWPNEWUSER:
			handler.HandleNewUser(nickDb, conn)
		case C.LPGP_LWPQUERYNICK:
			handler.HandleQueryNick(buf, conn, nickDb)
		case C.LPGP_LWPPUSHTOKEN:
			handler.HandlePushToken(buf, conn, serviceList)
		case C.LPGP_LWPGETLEADERBOARD:
			handler.HandleGetLeaderboard(buf, conn, serviceList)
		case C.LPGP_LWPSETNICKNAME:
			handler.HandleSetNickname(buf, conn)
		}
	}
	conn.Close()
	log.Printf("Conn closed %v", conn.RemoteAddr())
}
