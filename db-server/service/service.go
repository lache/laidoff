package service

import "github.com/gasbank/laidoff/db-server/user"

type Db interface {
	Create(args int, reply *user.Db) error
	Get(args *user.Id, reply *user.Db) error
	Lease(args *user.Id, reply *user.LeaseDb) error
	Write(args *user.LeaseDb, reply *int) error
}
