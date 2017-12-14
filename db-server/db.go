package main
import (
	"github.com/gasbank/laidoff/db-server/user"
	"log"
	"os"
)
func main() {
	// Create db directory to save user database
	os.MkdirAll("db", os.ModePerm)
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
