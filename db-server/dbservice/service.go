package dbservice

import (
	"github.com/gasbank/laidoff/db-server/user"
	"github.com/gasbank/laidoff/match-server/rpchelper"
)

type Db interface {
	Create(args int, reply *user.Db) error
	Get(args *user.Id, reply *user.Db) error
	Lease(args *user.Id, reply *user.LeaseDb) error
	Write(args *user.LeaseDb, reply *int) error
}

type Context struct {
	rpcContext *rpchelper.Context
}

func (c *Context) Create(args int, reply *user.Db) error {
	return c.rpcContext.Call("Create", args, reply)
}

func (c *Context) Get(args *user.Id, reply *user.Db) error {
	return c.rpcContext.Call("Get", args, reply)
}

func (c *Context) Lease(args *user.Id, reply *user.LeaseDb) error {
	return c.rpcContext.Call("Lease", args, reply)
}

func (c *Context) Write(args *user.LeaseDb, reply *int) error {
	return c.rpcContext.Call("Write", args, reply)
}

func New(address string) Db {
	c := new(Context)
	c.rpcContext = rpchelper.New("DbService", address)
	return c
}
