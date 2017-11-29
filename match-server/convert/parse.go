package convert

import (
	"bytes"
	"encoding/binary"
	"log"
)
// #include "../../src/puckgamepacket.h"
import "C"

func ParsePushToken(buf []byte) (*C.LWPPUSHTOKEN, error) {
	bufReader := bytes.NewReader(buf)
	recvPacket := &C.LWPPUSHTOKEN{}
	err := binary.Read(bufReader, binary.LittleEndian, recvPacket)
	if err != nil {
		log.Printf("binary.Read fail: %v", err.Error())
		return nil, err
	}
	return recvPacket, nil
}

func ParseQueryNick(buf []byte) (*C.LWPQUERYNICK, error) {
	bufReader := bytes.NewReader(buf)
	recvPacket := &C.LWPQUERYNICK{}
	err := binary.Read(bufReader, binary.LittleEndian, recvPacket)
	if err != nil {
		log.Printf("binary.Read fail: %v", err.Error())
		return nil, err
	}
	return recvPacket, nil
}

func ParseQueue2(buf []byte) (*C.LWPQUEUE2, error) {
	bufReader := bytes.NewReader(buf)
	recvPacket := &C.LWPQUEUE2{}
	err := binary.Read(bufReader, binary.LittleEndian, recvPacket)
	if err != nil {
		log.Printf("binary.Read fail: %v", err.Error())
		return nil, err
	}
	return recvPacket, nil
}

func ParseSetNickname(buf []byte) (*C.LWPSETNICKNAME, error) {
	bufReader := bytes.NewReader(buf)
	recvPacket := &C.LWPSETNICKNAME{}
	err := binary.Read(bufReader, binary.LittleEndian, recvPacket)
	if err != nil {
		log.Printf("binary.Read fail: %v", err.Error())
		return nil, err
	}
	return recvPacket, nil
}

func ParseGetLeaderboard(buf []byte) (*C.LWPGETLEADERBOARD, error) {
	bufReader := bytes.NewReader(buf)
	recvPacket := &C.LWPGETLEADERBOARD{}
	err := binary.Read(bufReader, binary.LittleEndian, recvPacket)
	if err != nil {
		log.Printf("binary.Read fail: %v", err.Error())
		return nil, err
	}
	return recvPacket, nil
}
