package main

import (
	"log"
	"net"
	//"time"
	"encoding/binary"
)

const (
	CONN_HOST = "0.0.0.0"
	CONN_PORT = "19856"
	CONN_TYPE = "tcp"

	LSBPT_LWSPHEREBATTLEPACKETQUEUE2 = 200
	LSBPT_LWSPHEREBATTLEPACKETMAYBEMATCHED = 201
	LSBPT_LWSPHEREBATTLEPACKETMATCHED2 = 202
	LSBPT_LWSPHEREBATTLEPACKETQUEUEOK = 203
	LSBPT_LWSPHEREBATTLEPACKETRETRYQUEUE = 204
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

func matchWorker(matchQueue <-chan net.Conn) {
	for {
		c1 := <- matchQueue
		c2 := <- matchQueue
		log.Printf("%v and %v matched! (maybe)", c1.RemoteAddr(), c2.RemoteAddr())
		buf := make([]byte, 4)
		binary.LittleEndian.PutUint16(buf, 4)
		binary.LittleEndian.PutUint16(buf[2:], LSBPT_LWSPHEREBATTLEPACKETMAYBEMATCHED) // matched (maybe)
		n1, err1 := c1.Write(buf)
		n2, err2 := c2.Write(buf)
		if n1 == 4 && n2 == 4 && err1 == nil && err2 == nil {
			// No error! proceed battle
			log.Printf("MATCH %v and %v matched successfully!", c1.RemoteAddr(), c2.RemoteAddr())
			buf := make([]byte, 2+2+4+2)
			binary.LittleEndian.PutUint16(buf[0:2], 2+2+4+2)
			binary.LittleEndian.PutUint16(buf[2:4], LSBPT_LWSPHEREBATTLEPACKETMATCHED2) // matched (maybe)
			// IP Address & ports
			buf[4] = 192
			buf[5] = 168
			buf[6] = 0
			buf[7] = 28
			binary.LittleEndian.PutUint16(buf[8:10], 10220)
			c1.Write(buf)
			c2.Write(buf)
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
