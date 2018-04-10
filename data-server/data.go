package main

import "C"
import (
	"fmt"
	"net"
	"os"
	"encoding/binary"
	"log"
	"unsafe"
	"path/filepath"
	"github.com/gasbank/laidoff/match-server/convert"
	"github.com/fsnotify/fsnotify"
	"time"
	"sync"
)

/*
#include <stdio.h>
#include <stdlib.h>
typedef struct _LWREMTEXTEXPART {
    unsigned int name_hash;
    unsigned int total_size;
    unsigned int offset;
    unsigned int payload_size;
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
	if len(os.Args) < 2 {
		log.Printf("Should provide resource glob as first argument. (i.e. \"c:/laidoff/assets/ktx/*.ktx\")")
		return
	}
	/* Lets prepare a address at any address at port 10001*/
	serverAddr, err := net.ResolveUDPAddr("udp", ":19876")
	checkError(err)

	/* Now listen at selected port */
	serverConn, err := net.ListenUDP("udp", serverAddr)
	checkError(err)
	defer serverConn.Close()

	var fileCacheMutex = &sync.Mutex{}
	fileCacheMap := make(map[uint32][]byte)

	var clientMutex = &sync.Mutex{}
	clientMap := make(map[*net.UDPAddr]time.Time)

	watcher := setupWatcher(fileCacheMutex, fileCacheMap, clientMutex, clientMap, serverConn)
	defer watcher.Close()

	readBuf := make([]byte, 1024)

	files, _ := filepath.Glob(os.Args[1])
	fileMap := make(map[uint32]string)
	log.Printf("Total data files: %v", len(files))
	jobMap := make(map[*net.UDPAddr]int)
	for _, f := range files {
		name, h := getNameHash(f)
		if oldName, exist := fileMap[h]; exist {
			log.Fatalf("Data name hash collision - old '%v' and new '%v'", oldName, name)
		}
		fileMap[h] = f
		log.Printf("data 0x%08X %v", h, name)
	}

	for {
		n, addr, err := serverConn.ReadFromUDP(readBuf)

		if err != nil {
			fmt.Println("Error: ", err)
		}

		clientMutex.Lock()
		clientMap[addr] = time.Now()
		clientMutex.Unlock()

		go func() {
			if n == 8 {
				fileCacheMutex.Lock()
				handleDataRequest(readBuf, fileMap, fileCacheMap, jobMap, addr, serverConn)
				fileCacheMutex.Unlock()
			} else {
				log.Printf("Unknown size: %v", n)
			}
		}()
	}
}

func setupWatcher(fileCacheMutex *sync.Mutex, fileCacheMap map[uint32][]byte,
	clientMutex *sync.Mutex, clientMap map[*net.UDPAddr]time.Time,
		serverConn *net.UDPConn) *fsnotify.Watcher {
	watcher, err := fsnotify.NewWatcher()
	if err != nil {
		log.Fatal(err)
	}
	go dataRefreshWatcher(watcher, fileCacheMutex, fileCacheMap, clientMutex, clientMap, serverConn)
	watchDir := filepath.Dir(os.Args[1])
	log.Printf("Watching %v", watchDir)
	err = watcher.Add(watchDir)
	if err != nil {
		log.Fatal(err)
	}
	return watcher
}

func getNameHash(f string) (string, uint32) {
	filename := filepath.Base(f)
	name := filename[:len(filename)-len(filepath.Ext(filename))]
	h := hash(name)
	return name, h
}

func dataRefreshWatcher(watcher *fsnotify.Watcher, fileCacheMutex *sync.Mutex, fileCacheMap map[uint32][]byte, clientMutex *sync.Mutex, clientMap map[*net.UDPAddr]time.Time, serverConn *net.UDPConn) {
	waitMutex := &sync.Mutex{}
	waitMap := make(map[string]chan bool)
	for {
		select {
		case event := <-watcher.Events:
			//log.Println("event:", event)
			if event.Op&fsnotify.Write == fsnotify.Write {
				//log.Println("modified file:", event.Name)
				var abortCh chan bool
				waitMutex.Lock()
				// abort previous refresh job
				if prevAbortCh, exist := waitMap[event.Name]; exist {
					prevAbortCh <- true
					abortCh = prevAbortCh
				} else {
					abortCh = make(chan bool, 1)
					waitMap[event.Name] = abortCh
				}
				waitMutex.Unlock()
				go func(abortCh chan bool, filename string) {
					select {
					case <-abortCh:
						// aborted; do nothing
					case <-time.After(1 * time.Second):
						name, hash := getNameHash(filename)
						log.Printf("File modified (stable): %v", filename)
						log.Printf("Invalidating cache for data 0x%08X %v...", hash, name)

						waitMutex.Lock()
						delete(waitMap, filename)
						waitMutex.Unlock()

						fileCacheMutex.Lock()
						delete(fileCacheMap, hash)
						fileCacheMutex.Unlock()

						clientMutex.Lock()
						for client := range clientMap {
							sendData(serverConn, client, hash, nil, 0, 0)
						}
						clientMutex.Unlock()

						close(abortCh)
					}
				}(abortCh, event.Name)
			}
		case err := <-watcher.Errors:
			log.Println("error:", err)
		}
	}
}

func handleDataRequest(buf []byte, fileMap map[uint32]string, fileCacheMap map[uint32][]byte, jobMap map[*net.UDPAddr]int, addr *net.UDPAddr, serverConn *net.UDPConn) {
	nameHash := binary.LittleEndian.Uint32(buf[0:4])
	offset := int(binary.LittleEndian.Uint32(buf[4:8]))
	//log.Printf("Received %v bytes from %v: nameHash %v, offset %v", n, addr, nameHash, offset)
	// check cache
	filename := fileMap[nameHash]
	if _, ok := fileCacheMap[nameHash]; ok {
		// already cached
	} else {
		log.Printf("%v: Not cached file.", filename)
		filename := fileMap[nameHash]
		file, err := os.Open(filename) // For read access.
		if err != nil {
			log.Fatalf(err.Error())
		}
		fileStat, err := file.Stat()
		if err != nil {
			log.Fatalf(err.Error())
		}
		totalSize := fileStat.Size()
		fileCacheMap[nameHash] = make([]byte, totalSize)
		_, err = file.Read(fileCacheMap[nameHash])
		if err != nil {
			log.Fatalf(err.Error())
		}
		log.Printf("%v: Saved. Total size %v bytes.", filename, len(fileCacheMap[nameHash]))
	}
	fileData := fileCacheMap[nameHash]
	totalSize := len(fileData)
	if _, ok := jobMap[addr]; ok {
		log.Printf("Ongoing job exists... skip this request")
	} else {
		// burst send
		currentOffset := offset
		burstMaxCount := 100
		burstCount := 0
		for sendData(serverConn, addr, nameHash, fileData, totalSize, currentOffset) == false {
			currentOffset = currentOffset + 1024
			burstCount++
			if burstCount >= burstMaxCount {
				break
			}
		}
		delete(jobMap, addr)
	}
}

func sendData(serverConn *net.UDPConn, addr *net.UDPAddr, nameHash uint32, fileData []byte, totalSize int, offset int) bool {
	payloadSize := 0
	remained := totalSize - offset
	last := false
	if remained < 0 {
		payloadSize = 0
		offset = totalSize
		last = true
	} else if remained <= 1024 {
		payloadSize = remained
		last = true
	} else {
		payloadSize = 1024
	}

	texPart := C.LWREMTEXTEXPART{}
	texPart.name_hash = C.uint(nameHash)
	texPart.total_size = C.uint(totalSize)
	texPart.payload_size = C.uint(payloadSize)
	texPart.offset = C.uint(offset)
	for i := 0; i < payloadSize; i++ {
		texPart.data[i] = C.uchar(fileData[offset+i])
	}

	replyBuf := convert.Packet2Buf(texPart)
	//log.Printf("Replying %v bytes to %v: file %v total %v offset %v, payloadSize %v", len(replyBuf), addr, filename, totalSize, offset, payloadSize)
	serverConn.WriteToUDP(replyBuf, addr)
	return last
}
