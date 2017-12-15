package main

import (
	"github.com/gasbank/laidoff/db-server/user"
	"log"
	"os"
	"net/rpc"
	"net"
	"github.com/gasbank/laidoff/match-server/nickdb"
	"errors"
	"time"
	"encoding/gob"
)

type LeaseData struct {
	LeaseId  user.LeaseId
	LeasedAt time.Time
}

type LeaseReply struct {
	LeaseDb *user.LeaseDb
	err     error
}

type LeaseWriteRequest struct {
	Id             *user.Id
	LeaseReplyChan chan LeaseReply
	LeaseDb        *user.LeaseDb
	WriteReplyChan chan WriteReply
}

type WriteReply struct {
	Reply int
	err   error
}

type DbService struct {
	nickDb                 nickdb.NickDb
	leaseMap               map[user.Id]LeaseData
	leaseWriteRequestQueue chan LeaseWriteRequest
}

func (t *DbService) Create(args int, reply *user.Db) error {
	// create a new test user and write db to filesystem
	uuid, uuidStr, err := user.NewUuid()
	if err != nil {
		log.Printf("[Service] new uuid failed: %v", err.Error())
		return err
	}
	newNick := nickdb.PickRandomNick(&t.nickDb)
	// Write to disk
	var id user.Id
	copy(id[:], uuid)
	userDb, _, err := createNewUser(id, newNick)
	if err != nil {
		log.Printf("[Service] createNewUser failed: %v", err.Error())
		return err
	}
	*reply = *userDb
	log.Printf("[Service] Create ok (user ID %v, %v)", uuidStr, uuid)
	return nil
}

func (t *DbService) Get(args *user.Id, reply *user.Db) error {
	db, err := loadUserDb(*args)
	if err != nil {
		return err
	}
	log.Printf("[Service] Get ok (user ID %v)", args)
	*reply = *db
	return nil
}

func (t *DbService) Lease(args *user.Id, reply *user.LeaseDb) error {
	replyChan := make(chan LeaseReply)
	t.leaseWriteRequestQueue <- LeaseWriteRequest{args, replyChan, nil, nil}
	r := <-replyChan
	if r.err != nil {
		return r.err
	} else {
		*reply = *r.LeaseDb
	}
	return nil
}

func CommitLease(t *DbService, leaseRequest *LeaseWriteRequest) {
	args := *leaseRequest.Id
	leaseData, exist := t.leaseMap[args]
	if exist == false {
		// You can lease
		db, err := loadUserDb(args)
		if err != nil {
			leaseRequest.LeaseReplyChan <- LeaseReply{nil, err}
			return
		}
		leaseIdSlice, _, err := user.NewUuid()
		if err != nil {
			leaseRequest.LeaseReplyChan <- LeaseReply{nil, err}
			return
		}
		var leaseId user.LeaseId
		copy(leaseId[:], leaseIdSlice)
		t.leaseMap[args] = LeaseData{leaseId, time.Now()}
		leaseRequest.LeaseReplyChan <- LeaseReply{&user.LeaseDb{leaseId, *db}, nil}
		log.Printf("[Service] Lease ok (user ID %v)", args)
	} else {
		leaseDuration := time.Now().Sub(leaseData.LeasedAt)
		if leaseDuration > 1 * time.Second {
			log.Printf("Lease data already exists, but it is too old. (%v) Force remove lease data...", leaseDuration)
			delete(t.leaseMap, args)
			CommitLease(t, leaseRequest)
			return
		}
		leaseRequest.LeaseReplyChan <- LeaseReply{nil, errors.New("already leased")}
		return
	}
}

func (t *DbService) Write(args *user.LeaseDb, reply *int) error {
	replyChan := make(chan WriteReply)
	t.leaseWriteRequestQueue <- LeaseWriteRequest{nil, nil, args, replyChan}
	r := <-replyChan
	if r.err != nil {
		return r.err
	} else {
		*reply = r.Reply
	}
	return nil
}

func CommitWrite(t *DbService, writeRequest *LeaseWriteRequest) {
	leaseData, exist := t.leaseMap[writeRequest.LeaseDb.Db.Id]
	if exist {
		if leaseData.LeaseId == writeRequest.LeaseDb.LeaseId {
			err := writeUserDb(&writeRequest.LeaseDb.Db)
			if err != nil {
				log.Printf("writeUserDb failed with error: %v", err.Error())
				writeRequest.WriteReplyChan <- WriteReply{0, err}
				return
			}
			delete(t.leaseMap, writeRequest.LeaseDb.Db.Id)
			log.Printf("[Service] Write user ok (nickname %v, user ID %v)",
				writeRequest.LeaseDb.Db.Nickname, writeRequest.LeaseDb.Db.Id)
			writeRequest.WriteReplyChan <- WriteReply{1, nil}
		} else {
			writeRequest.WriteReplyChan <- WriteReply{0, errors.New("lease not match")}
			return
		}
	} else {
		writeRequest.WriteReplyChan <- WriteReply{0, errors.New("lease not found")}
		return
	}
}

func ProcessLeaseWriteRequest(t *DbService) {
	for {
		request := <-t.leaseWriteRequestQueue
		if request.LeaseReplyChan != nil {
			CommitLease(t, &request)
		} else if request.WriteReplyChan != nil {
			CommitWrite(t, &request)
		}
	}
}

func main() {
	// Create db directory to save user database
	os.MkdirAll("db", os.ModePerm)
	//createTestUserDb()
	server := rpc.NewServer()
	dbService := new(DbService)
	dbService.nickDb = nickdb.LoadNickDb()
	dbService.leaseMap = make(map[user.Id]LeaseData)
	dbService.leaseWriteRequestQueue = make(chan LeaseWriteRequest)
	go ProcessLeaseWriteRequest(dbService)
	server.RegisterName("DbService", dbService)
	addr := ":20181"
	log.Printf("Listening %v for db service...", addr)
	// Listen for incoming tcp packets on specified port.
	l, e := net.Listen("tcp", addr)
	if e != nil {
		log.Fatal("listen error:", e)
	}
	// This statement links rpc server to the socket, and allows rpc server to accept
	// rpc request coming from that socket.
	server.Accept(l)
}
func createTestUserDb() {
	// create a new test user and write db to filesystem
	uuid, uuidStr, err := user.NewUuid()
	if err != nil {
		log.Fatalf("new uuid failed: %v", err.Error())
	}
	log.Printf("  - New user guid: %v", uuidStr)
	newNick := "test-user-nick"
	// Write to disk
	var id user.Id
	copy(id[:], uuid)
	_, _, err = createNewUser(id, newNick)
	if err != nil {
		log.Fatalf("createNewUser failed: %v", err.Error())
	}
}

func loadUserDb(id user.Id) (*user.Db, error) {
	uuidStr := user.IdByteArrayToString(id)
	userDbFile, err := os.Open("db/" + uuidStr)
	if err != nil {
		if os.IsNotExist(err) {
			// user db not exist
			return nil, errors.New("user db not exist")
		} else {
			log.Printf("disk open failed: %v", err.Error())
			return nil, err
		}
	} else {
		defer userDbFile.Close()
		decoder := gob.NewDecoder(userDbFile)
		userDb := &user.Db{}
		decoder.Decode(userDb)
		return userDb, nil
	}
}

func createNewUser(uuid user.Id, nickname string) (*user.Db, *os.File, error) {
	userDb := &user.Db{
		Id:       uuid,
		Created:  time.Now(),
		Nickname: nickname,
	}
	uuidStr := user.IdByteArrayToString(uuid)
	userDbFile, err := os.Create("db/" + uuidStr)
	if err != nil {
		log.Fatalf("User db file creation failed: %v", err.Error())
		return nil, nil, err
	}
	encoder := gob.NewEncoder(userDbFile)
	encoder.Encode(userDb)
	userDbFile.Close()
	return userDb, userDbFile, nil
}

func writeUserDb(userDb *user.Db) error {
	userDbFile, err := os.Create("db/" + user.IdByteArrayToString(userDb.Id))
	if err != nil {
		log.Fatalf("User db file creation failed: %v", err.Error())
		return err
	}
	encoder := gob.NewEncoder(userDbFile)
	encoder.Encode(userDb)
	userDbFile.Close()
	log.Printf("DB written: %+v", userDb)
	return nil
}
