package main

import "C"
import (
	"fmt"
	"net"
	"os"
	"encoding/binary"
	"log"
	"github.com/gasbank/laidoff/match-server/convert"
	"unsafe"
	"path/filepath"
)

/*
#include <stdio.h>
#include <stdlib.h>
typedef struct _LWREMTEXTEXPART {
    unsigned int namehash;
    unsigned short w;
    unsigned short h;
    unsigned short len;
    unsigned short padding;
    unsigned int offset;
    unsigned char data[1024];
} LWREMTEXTEXPART;

unsigned long hash2(const unsigned char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; // hash * 33 + c

	return hash;
}
unsigned long hash(const char *str) {
	return hash2((const unsigned char*)str);
}
*/
import "C"

/* A Simple function to verify error */
func checkError(err error) {
	if err != nil {
		fmt.Println("Error: ", err)
		os.Exit(0)
	}
}

func hash(s string) uint32 {
	cStr := C.CString(s)
	h := C.hash(cStr)
	C.free(unsafe.Pointer(cStr))
	return uint32(h)
}

func main() {
	/* Lets prepare a address at any address at port 10001*/
	serverAddr, err := net.ResolveUDPAddr("udp", ":19876")
	checkError(err)

	/* Now listen at selected port */
	serverConn, err := net.ListenUDP("udp", serverAddr)
	checkError(err)
	defer serverConn.Close()

	buf := make([]byte, 1024)

	cstr := C.CString("tex1")
	texhash := C.hash(cstr)
	C.free(unsafe.Pointer(cstr))
	log.Printf("Hash: %v", texhash)

	files, _ := filepath.Glob("C:/laidoff/assets/ktx/*.ktx")
	fileMap := make(map[uint32]string)
	for _, f := range files {
		filename := filepath.Base(f)
		name := filename[:len(filename)-len(filepath.Ext(filename))]
		h := hash(name)
		fileMap[h] = f
		log.Printf("filename: %v, hash: %v", name, h)
	}

	for {
		n, addr, err := serverConn.ReadFromUDP(buf)

		if err != nil {
			fmt.Println("Error: ", err)
		}

		if n == 8 {
			nameHash := binary.LittleEndian.Uint32(buf[0:4])
			offset := binary.LittleEndian.Uint32(buf[4:8])
			log.Printf("Received %v bytes from %v: nameHash %v, offset %v", n, addr, nameHash, offset)


			file, err := os.Open(fileMap[nameHash]) // For read access.
			if err != nil {
				log.Fatalf(err.Error())
			}
			data := make([]byte, 12 + (4 * 8))
			_, err = file.Read(data)
			w := binary.LittleEndian.Uint32(data[12 + (4 * 6):12 + (4 * 7)])
			h := binary.LittleEndian.Uint32(data[12 + (4 * 7):12 + (4 * 8)])
			totalLen := uint32(w * h * 3)
			length := uint32(0)
			remained := totalLen - offset
			if remained < 1024 {
				length = remained
			} else {
				length = 1024
			}

			texPart := C.LWREMTEXTEXPART{}
			texPart.namehash = C.uint(nameHash)
			texPart.w = C.ushort(w)
			texPart.h = C.ushort(h)
			texPart.len = C.ushort(length)
			texPart.offset = C.uint(offset)
			for i := uint32(0); i < length; i++ {
				texPart.data[i] = C.uchar(i)
			}

			replyBuf := convert.Packet2Buf(texPart)
			log.Printf("Replying %v bytes to %v: offset %v, length %v", len(replyBuf), addr, offset, length)
			serverConn.WriteToUDP(replyBuf, addr)
		} else {
			log.Printf("Unknown size: %v", n)
		}
	}
}
