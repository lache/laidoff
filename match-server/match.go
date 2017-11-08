package main

import (
	"log"
	"net"
	"encoding/binary"
	"bytes"
	"unsafe"
)

// #include "../src/puckgamepacket.h"
import "C"

const (
	ConnHost = "0.0.0.0"
	ConnPort = "19856"
	ConnType = "tcp"
	BattleServiceHost = "192.168.0.28"
	//BattleServiceHost = "221.147.71.76"
	BattleServicePort        = "29856"
	BattleServiceConnType    = "tcp"
	BattleServiceUserPort = 10220
)

func testC() {
	log.Printf("%d\n", C.LPGP_LWPCREATEBATTLEOK)
	tt := C.LWPCREATEBATTLEOK{
		C.ushort(unsafe.Sizeof(C.LPGP_LWPCREATEBATTLEOK)),
		C.LPGP_LWPCREATEBATTLEOK,
		1,
	}
	ttBuf := &bytes.Buffer{}
	binary.Write(ttBuf, binary.LittleEndian, tt)
	log.Println(tt)
	log.Printf("ttBuf %+v len: %v", ttBuf, ttBuf.Len())
}

func main() {
	log.Println("Greetings from match server")
	matchQueue := make(chan net.Conn)
	l, err := net.Listen(ConnType, ConnHost+ ":" +ConnPort)
	if err != nil {
		log.Fatalln("Error listening:", err.Error())
	}
	defer l.Close()
	log.Println("Listening on " + ConnHost + ":" + ConnPort)
	go matchWorker(matchQueue)
	for {
		conn, err := l.Accept()
		if err != nil {
			log.Println("Error accepting: ", err.Error())
		} else {
			go handleRequest(conn, matchQueue)
		}
	}
}

func int2ByteArray(v C.int) [4]byte {
	var byteArray [4]byte
	binary.LittleEndian.PutUint32(byteArray[0:], uint32(v))
	return byteArray
}

func createBattleInstance(c1 net.Conn, c2 net.Conn) {
	// Connect to battle service
	tcpAddr, err := net.ResolveTCPAddr(BattleServiceConnType, BattleServiceHost + ":" + BattleServicePort)
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
		tcpAddrIpv4 := tcpAddr.IP.To4()
		matched2Buf := packet2Buf(C.LWPMATCHED2 {
			C.ushort(unsafe.Sizeof(C.LWPMATCHED2{})),
			C.LPGP_LWPMATCHED2,
			C.ushort(BattleServiceUserPort),
			C.ushort(0), // padding
			[4]C.uchar{C.uchar(tcpAddrIpv4[0]),C.uchar(tcpAddrIpv4[1]),C.uchar(tcpAddrIpv4[2]),C.uchar(tcpAddrIpv4[3]),},
			createBattleOk.Battle_id,
		})
		c1.Write(matched2Buf)
		c2.Write(matched2Buf)
	} else {
		log.Printf("Recv LSBPT_LWSPHEREBATTLEPACKETCREATEBATTLE reply corrupted")
	}
}
func packet2Buf(packet interface{}) []byte {
	buf := &bytes.Buffer{}
	binary.Write(buf, binary.LittleEndian, packet)
	return buf.Bytes()
}

func matchWorker(matchQueue <-chan net.Conn) {
	for {
		c1 := <- matchQueue
		c2 := <- matchQueue
		log.Printf("%v and %v matched! (maybe)", c1.RemoteAddr(), c2.RemoteAddr())
		maybeMatchedBuf := packet2Buf(&C.LWPMAYBEMATCHED{
			C.ushort(unsafe.Sizeof(C.LWPMAYBEMATCHED{})),
			C.LPGP_LWPMAYBEMATCHED,
		})
		n1, err1 := c1.Write(maybeMatchedBuf)
		n2, err2 := c2.Write(maybeMatchedBuf)
		if n1 == 4 && n2 == 4 && err1 == nil && err2 == nil {
			go createBattleInstance(c1, c2)
		} else {
			// Match cannot be proceeded
			checkMatchError(err1, c1)
			checkMatchError(err2, c2)
		}
	}
}

func checkMatchError(err error, conn net.Conn) {
	if err != nil {
		log.Printf("%v: %v error!", conn.RemoteAddr(), err.Error())
	} else {
		retryQueueBuf := packet2Buf(&C.LWPRETRYQUEUE{
			C.ushort(unsafe.Sizeof(C.LWPRETRYQUEUE{})),
			C.LPGP_LWPRETRYQUEUE,
		})
		_, retrySendErr := conn.Write(retryQueueBuf)
		if retrySendErr != nil {
			log.Printf("%v: %v error!", conn.RemoteAddr(), retrySendErr.Error())
		} else {
			log.Printf("%v: do retry match", conn.RemoteAddr())
		}
	}
}

func handleRequest(conn net.Conn, matchQueue chan<- net.Conn) {
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
		}
	}
	conn.Close()
	log.Printf("Conn closed %v", conn.RemoteAddr())
}
