package main

import (
	"log"
	"net"
	"encoding/binary"
	"bytes"
	"unsafe"
	"os"
	"encoding/json"
)

// #include "../src/puckgamepacket.h"
import "C"

type ServerConfig struct {
	ConnHost string
	ConnPort string
	ConnType string

	BattleServiceHost string
	BattleServicePort string
	BattleServiceConnType string

	BattlePublicServiceHost string
	BattlePublicServicePort string
	BattlePublicServiceConnType string
}

func main() {
	log.Println("Greetings from match server")
	confFile, err := os.Open("conf.json")
	if err != nil {
		log.Fatalf("conf.json open error:%v", err.Error())
	}
	confFileDecoder := json.NewDecoder(confFile)
	conf := ServerConfig{}
	err = confFileDecoder.Decode(&conf)
	if err != nil {
		log.Fatalf("conf.json parse error:%v", err.Error())
	}
	matchQueue := make(chan net.Conn)
	l, err := net.Listen(conf.ConnType, conf.ConnHost + ":" + conf.ConnPort)
	if err != nil {
		log.Fatalln("Error listening:", err.Error())
	}
	defer l.Close()
	log.Println("Listening on " + conf.ConnHost + ":" + conf.ConnPort)
	go matchWorker(conf, matchQueue)
	for {
		conn, err := l.Accept()
		if err != nil {
			log.Println("Error accepting: ", err.Error())
		} else {
			go handleRequest(conf, conn, matchQueue)
		}
	}
}

func int2ByteArray(v C.int) [4]byte {
	var byteArray [4]byte
	binary.LittleEndian.PutUint32(byteArray[0:], uint32(v))
	return byteArray
}

func createBattleInstance(conf ServerConfig, c1 net.Conn, c2 net.Conn) {
	// Connect to battle service
	tcpAddr, err := net.ResolveTCPAddr(conf.BattleServiceConnType, conf.BattleServiceHost + ":" + conf.BattleServicePort)
	if err != nil {
		log.Fatalf("ResolveTCPAddr error! - %v", err.Error())
	}
	conn, err := net.DialTCP("tcp", nil, tcpAddr)
	if err != nil {
		log.Fatalf("DialTCP error! - %v", err.Error())
	}
	// Send create battle request
	createBattleBuf := packet2Buf(&C.LWPCREATEBATTLE{
		C.ushort(unsafe.Sizeof(C.LWPCREATEBATTLE{})),
		C.LPGP_LWPCREATEBATTLE,
	})
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
		log.Printf("MATCH %v and %v matched successfully!", c1.RemoteAddr(), c2.RemoteAddr())
		c1.Write(createMatched2Buf(conf, createBattleOk, createBattleOk.C1_token))
		c2.Write(createMatched2Buf(conf, createBattleOk, createBattleOk.C2_token))
	} else {
		log.Printf("Recv LSBPT_LWSPHEREBATTLEPACKETCREATEBATTLE reply corrupted")
	}
}
func createMatched2Buf(conf ServerConfig, createBattleOk C.LWPCREATEBATTLEOK, token C.uint) []byte {
	publicAddr, err := net.ResolveTCPAddr(conf.BattlePublicServiceConnType, conf.BattlePublicServiceHost + ":" + conf.BattlePublicServicePort)
	if err != nil {
		log.Panicf("BattlePublicService conf parse error: %v", err.Error())
	}
	publicAddrIpv4 := publicAddr.IP.To4()
	return packet2Buf(C.LWPMATCHED2{
		C.ushort(unsafe.Sizeof(C.LWPMATCHED2{})),
		C.LPGP_LWPMATCHED2,
		C.ushort(publicAddr.Port), // createBattleOk.Port
		C.ushort(0), // padding
		[4]C.uchar{C.uchar(publicAddrIpv4[0]),C.uchar(publicAddrIpv4[1]),C.uchar(publicAddrIpv4[2]),C.uchar(publicAddrIpv4[3]),},
		createBattleOk.Battle_id,
		token,
	})
}
func packet2Buf(packet interface{}) []byte {
	buf := &bytes.Buffer{}
	binary.Write(buf, binary.LittleEndian, packet)
	return buf.Bytes()
}

func matchWorker(conf ServerConfig, matchQueue <-chan net.Conn) {
	for {
		c1 := <- matchQueue
		c2 := <- matchQueue
		if c1 == c2 {
			sendRetryQueue(c1)
		} else {
			log.Printf("%v and %v matched! (maybe)", c1.RemoteAddr(), c2.RemoteAddr())
			maybeMatchedBuf := packet2Buf(&C.LWPMAYBEMATCHED{
				C.ushort(unsafe.Sizeof(C.LWPMAYBEMATCHED{})),
				C.LPGP_LWPMAYBEMATCHED,
			})
			n1, err1 := c1.Write(maybeMatchedBuf)
			n2, err2 := c2.Write(maybeMatchedBuf)
			if n1 == 4 && n2 == 4 && err1 == nil && err2 == nil {
				go createBattleInstance(conf, c1, c2)
			} else {
				// Match cannot be proceeded
				checkMatchError(err1, c1)
				checkMatchError(err2, c2)
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
	retryQueueBuf := packet2Buf(&C.LWPRETRYQUEUE{
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

func handleRequest(conf ServerConfig, conn net.Conn, matchQueue chan<- net.Conn) {
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
			log.Printf("QUEUE2 received")
			matchQueue <- conn
			queueOkBuf := packet2Buf(&C.LWPQUEUEOK{
				C.ushort(unsafe.Sizeof(C.LWPQUEUEOK{})),
				C.LPGP_LWPQUEUEOK,
			})
			conn.Write(queueOkBuf)
		case C.LPGP_LWPSUDDENDEATH:
			log.Printf("SUDDENDEATH received")
			tcpAddr, err := net.ResolveTCPAddr(conf.BattleServiceConnType, conf.BattleServiceHost + ":" + conf.BattleServicePort)
			if err != nil {
				log.Fatalf("ResolveTCPAddr error! - %v", err.Error())
			}
			conn, err := net.DialTCP("tcp", nil, tcpAddr)
			if err != nil {
				log.Fatalf("DialTCP error! - %v", err.Error())
			}
			_, err = conn.Write(buf)
			if err != nil {
				log.Fatalf("Send SUDDENDEATH failed")
			}
		}
	}
	conn.Close()
	log.Printf("Conn closed %v", conn.RemoteAddr())
}
