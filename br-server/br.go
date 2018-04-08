package main

import (
	dbentry "github.com/gasbank/laidoff/db-server/entry"
	matchentry "github.com/gasbank/laidoff/match-server/entry"
	pushentry "github.com/gasbank/laidoff/push-server/entry"
	rankentry "github.com/gasbank/laidoff/rank-server/entry"
	rewardentry "github.com/gasbank/laidoff/reward-server/entry"
)

func main() {
	go dbentry.Entry()
	go matchentry.Entry()
	go pushentry.Entry()
	go rankentry.Entry()
	go rewardentry.Entry()
}
