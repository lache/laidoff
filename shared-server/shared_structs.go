package shared_server

type Args struct {
	A, B int
}

type Quotient struct {
	Quo, Rem int
}

type PushToken struct {
	Domain int
	PushToken string
	UserId []byte
}
