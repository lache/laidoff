package service

import "github.com/gasbank/laidoff/db-server/user"

type Db interface {
	Create(args *user.Db, reply *int) error
	Read(args *user.Id, reply *user.Db) error
	Update(args *user.Db, reply *int) error
	Delete(args *user.Db) error
}
