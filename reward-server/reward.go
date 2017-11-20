package main

import (
	//"../shared-server"
	"log"
	"net"
	"./helpers"
	"encoding/binary"
	"bytes"
)

// #include "../src/puckgamepacket.h"
import "C"

const (
	ServiceName = "reward"
	ServiceAddr = ":10290"
	RankServiceAddr = ":20172"
)

func main() {
	// Set default log format
	log.SetFlags(log.Lshortfile | log.LstdFlags)
	log.Printf("Greetings from %v service", ServiceName)
	// Connect to rank service
	rank := helpers.NewRankClient(RankServiceAddr)
	// Start listening
	l, err := net.Listen("tcp", ServiceAddr)
	if err != nil {
		log.Fatalln("Error listening:", err.Error())
	}
	defer l.Close()
	log.Printf("Listening %v for %v service...", ServiceAddr, ServiceName)
	for {
		conn, err := l.Accept()
		if err != nil {
			log.Println("Error accepting: ", err.Error())
		} else {
			go handleRequest(conn, rank)
		}
	}
}
func handleRequest(conn net.Conn, client *helpers.RankClient) {
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
		case C.LPGP_LWPBATTLERESULT:
			handleBattleResult(buf, conn)
		}
	}
	conn.Close()
	log.Printf("Conn closed %v", conn.RemoteAddr())
}

func handleBattleResult(buf []byte, conn net.Conn) {
	log.Printf("BATTLERESULT received")
	// Parse
	bufReader := bytes.NewReader(buf)
	recvPacket := C.LWPBATTLERESULT{}
	err := binary.Read(bufReader, binary.LittleEndian, &recvPacket)
	if err != nil {
		log.Printf("binary.Read fail: %v", err.Error())
		return
	}
	id1 := IdCuintToByteArray(recvPacket.Id1)
	id2 := IdCuintToByteArray(recvPacket.Id2)
	winner := int(recvPacket.Winner)
	log.Printf("Battle result received; id1=%v, id2=%v, winner=%v", id1, id2, winner)
}

type UserId [16]byte

func IdCuintToByteArray(id [4]C.uint) UserId {
	var b UserId
	binary.BigEndian.PutUint32(b[0:], uint32(id[0]))
	binary.BigEndian.PutUint32(b[4:], uint32(id[1]))
	binary.BigEndian.PutUint32(b[8:], uint32(id[2]))
	binary.BigEndian.PutUint32(b[12:], uint32(id[3]))
	return b
}
