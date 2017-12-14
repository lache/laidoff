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
)

type LeaseData struct {
	LeaseId  user.LeaseId
	LeasedAt time.Time
}

type DbService struct {
	nickDb   nickdb.NickDb
	leaseMap map[user.Id]LeaseData
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
	userDb, _, err := user.CreateNewUser(id, newNick)
	if err != nil {
		log.Printf("[Service] CreateNewUser failed: %v", err.Error())
		return err
	}
	*reply = *userDb
	log.Printf("[Service] Create ok (user ID %v, %v)", uuidStr, uuid)
	return nil
}

func (t *DbService) Get(args *user.Id, reply *user.Db) error {
	db, err := user.LoadUserDb(*args)
	if err != nil {
		return err
	}
	log.Printf("[Service] Get ok (user ID %v)", args)
	*reply = *db
	return nil
}

func (t *DbService) Lease(args *user.Id, reply *user.LeaseDb) error {
	_, exist := t.leaseMap[*args]
	if exist == false {
		// You can lease
		db, err := user.LoadUserDb(*args)
		if err != nil {
			return err
		}
		leaseIdSlice, _, err := user.NewUuid()
		if err != nil {
			return err
		}
		var leaseId user.LeaseId
		copy(leaseId[:], leaseIdSlice)
		t.leaseMap[*args] = LeaseData{leaseId, time.Now()}
		reply.Db = *db
		reply.LeaseId = leaseId
		log.Printf("[Service] Lease ok (user ID %v)", args)
	} else {
		return errors.New("already leased")
	}
	return nil
}

func (t *DbService) Write(args *user.LeaseDb, reply *int) error {
	leaseData, exist := t.leaseMap[args.Db.Id]
	if exist {
		if leaseData.LeaseId == args.LeaseId {
			err := user.WriteUserDb(&args.Db)
			if err != nil {
				log.Printf("WriteUserDb failed with error: %v", err.Error())
				return err
			}
			delete(t.leaseMap, args.Db.Id)
			log.Printf("[Service] Write user ok (nickname %v, user ID %v)", args.Db.Nickname, args.Db.Id)
			*reply = 1
		} else {
			return errors.New("lease not match")
		}
	} else {
		return errors.New("lease not found")
	}
	return nil
}

func main() {
	// Create db directory to save user database
	os.MkdirAll("db", os.ModePerm)
	createTestUserDb()
	server := rpc.NewServer()
	dbService := new(DbService)
	dbService.nickDb = nickdb.LoadNickDb()
	dbService.leaseMap = make(map[user.Id]LeaseData)
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
	_, _, err = user.CreateNewUser(id, newNick)
	if err != nil {
		log.Fatalf("CreateNewUser failed: %v", err.Error())
	}
}
