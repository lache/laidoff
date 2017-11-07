package main

import (
	"log"
	"net"
	"encoding/binary"
)

const (
	CONN_HOST = "0.0.0.0"
	CONN_PORT = "19856"
	CONN_TYPE = "tcp"

	BATTLE_SERVICE_HOST = "192.168.0.28"
	BATTLE_SERVICE_PORT = "29856"
	BATTLE_SERVICE_CONN_TYPE = "tcp"
	BATTLE_SERVICE_USER_PORT = 10288 // udp

	LSBPT_LWSPHEREBATTLEPACKETQUEUE2 = 200
	LSBPT_LWSPHEREBATTLEPACKETMAYBEMATCHED = 201
	LSBPT_LWSPHEREBATTLEPACKETMATCHED2 = 202
	LSBPT_LWSPHEREBATTLEPACKETQUEUEOK = 203
	LSBPT_LWSPHEREBATTLEPACKETRETRYQUEUE = 204

	LSBPT_LWSPHEREBATTLEPACKETCREATEBATTLE = 1000
	LSBPT_LWSPHEREBATTLEPACKETCREATEBATTLEOK = 1001
)

func main() {
	log.Println("Hello", "My", "Friend")
	matchQueue := make(chan net.Conn)
	l, err := net.Listen(CONN_TYPE, CONN_HOST + ":" + CONN_PORT)
	if err != nil {
		log.Fatalln("Error listening:", err.Error())
	}
	defer l.Close()
	log.Println("Listening on " + CONN_HOST + ":" + CONN_PORT)
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

func createBattleInstance(c1 net.Conn, c2 net.Conn) {
	// Connect to battle service
	tcpAddr, err := net.ResolveTCPAddr(BATTLE_SERVICE_CONN_TYPE, BATTLE_SERVICE_HOST + ":" + BATTLE_SERVICE_PORT)
	if err != nil {
		log.Fatalf("ResolveTCPAddr error! - %v", err.Error())
	}
	conn, err := net.DialTCP("tcp", nil, tcpAddr)
	if err != nil {
		log.Fatalf("DialTCP error! - %v", err.Error())
	}
	// Send create battle request
	bufSize := 4
	buf := make([]byte, bufSize)
	binary.LittleEndian.PutUint16(buf, uint16(bufSize))
	binary.LittleEndian.PutUint16(buf[2:], LSBPT_LWSPHEREBATTLEPACKETCREATEBATTLE) // matched (maybe)
	_, err = conn.Write(buf)
	if err != nil {
		log.Fatalf("Send LSBPT_LWSPHEREBATTLEPACKETCREATEBATTLE failed")
	}
	// Wait for a reply
	readBufSize := 8
	readBuf := make([]byte, readBufSize)
	readBufLen, err := conn.Read(readBuf)
	if err != nil {
		log.Fatalf("Recv LSBPT_LWSPHEREBATTLEPACKETCREATEBATTLE reply failed")
	}
	if readBufLen != 8 {
		log.Fatalf("Recv LSBPT_LWSPHEREBATTLEPACKETCREATEBATTLE reply size error")
	}
	replyLen := binary.LittleEndian.Uint16(readBuf[0:2])
	replyType := binary.LittleEndian.Uint16(readBuf[2:4])
	battleId := binary.LittleEndian.Uint32(readBuf[4:8])
	if replyLen == 8 && replyType == LSBPT_LWSPHEREBATTLEPACKETCREATEBATTLEOK {
		// No error! so far ... proceed battle
		log.Printf("MATCH %v and %v matched successfully!", c1.RemoteAddr(), c2.RemoteAddr())
		bufSize := 2+2+4+2+4
		buf := make([]byte, bufSize)
		binary.LittleEndian.PutUint16(buf[0:2], uint16(bufSize))
		binary.LittleEndian.PutUint16(buf[2:4], LSBPT_LWSPHEREBATTLEPACKETMATCHED2) // matched (maybe)
		// Send a battle server IP address & port to clients
		// [192.168.0.28:10220]
		buf[4] = tcpAddr.IP[0]
		buf[5] = tcpAddr.IP[1]
		buf[6] = tcpAddr.IP[2]
		buf[7] = tcpAddr.IP[3]
		binary.LittleEndian.PutUint16(buf[8:10], uint16(BATTLE_SERVICE_USER_PORT))
		binary.LittleEndian.PutUint32(buf[10:14], battleId)
		c1.Write(buf)
		c2.Write(buf)
	} else {
		log.Printf("Recv LSBPT_LWSPHEREBATTLEPACKETCREATEBATTLE reply corrupted")
	}
}

func matchWorker(matchQueue <-chan net.Conn) {
	for {
		c1 := <- matchQueue
		c2 := <- matchQueue
		log.Printf("%v and %v matched! (maybe)", c1.RemoteAddr(), c2.RemoteAddr())
		bufSize := 4
		buf := make([]byte, bufSize)
		binary.LittleEndian.PutUint16(buf, uint16(bufSize))
		binary.LittleEndian.PutUint16(buf[2:], LSBPT_LWSPHEREBATTLEPACKETMAYBEMATCHED) // matched (maybe)
		n1, err1 := c1.Write(buf)
		n2, err2 := c2.Write(buf)
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
		buf := make([]byte, 4)
		binary.LittleEndian.PutUint16(buf, 4)
		binary.LittleEndian.PutUint16(buf[2:], LSBPT_LWSPHEREBATTLEPACKETRETRYQUEUE) // do retry match
		_, retrySendErr := conn.Write(buf)
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
		case LSBPT_LWSPHEREBATTLEPACKETQUEUE2:
			log.Printf("QUEUE2 received")
			matchQueue <- conn
			buf := make([]byte, 4)
			binary.LittleEndian.PutUint16(buf[0:], 4)
			binary.LittleEndian.PutUint16(buf[2:], LSBPT_LWSPHEREBATTLEPACKETQUEUEOK)
			conn.Write(buf)
		}
	}
	conn.Close()
	log.Printf("Conn closed %v", conn.RemoteAddr())
}
