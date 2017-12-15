package main

import (
	"log"
	"net"
	"github.com/gasbank/laidoff/reward-server/helpers"
	"github.com/gasbank/laidoff/match-server/convert"
	"encoding/binary"
	"bytes"
	"time"
	"github.com/gasbank/laidoff/db-server/dbservice"
	"github.com/gasbank/laidoff/db-server/user"
)

const (
	ServiceName     = "reward"
	ServiceAddr     = ":10290"
	RankServiceAddr = ":20172"
)

func main() {
	// Set default log format
	log.SetFlags(log.Lshortfile | log.LstdFlags)
	log.Printf("Greetings from %v service", ServiceName)
	// Connect to rank service
	rank := helpers.NewRankClient(RankServiceAddr)
	// Start listening
	l, err := net.Listen("tcp", ServiceAddr)
	if err != nil {
		log.Fatalln("Error listening:", err.Error())
	}
	defer l.Close()
	dbService := dbservice.New(":20181")
	log.Printf("Listening %v for %v service...", ServiceAddr, ServiceName)
	for {
		conn, err := l.Accept()
		if err != nil {
			log.Println("Error accepting: ", err.Error())
		} else {
			go handleRequest(conn, rank, dbService)
		}
	}
}
func handleRequest(conn net.Conn, rank *helpers.RankClient, dbService dbservice.Db) {
	log.Printf("Accepting from %v", conn.RemoteAddr())
	for {
		buf := make([]byte, 1024)
		//conn.SetReadDeadline(time.Now().Add(10000 * time.Millisecond))
		readLen, err := conn.Read(buf)
		if readLen == 0 {
			log.Printf("%v: readLen is zero.", conn.RemoteAddr())
			break
		}
		if err != nil {
			log.Printf("%v: Error reading: %v", conn.RemoteAddr(), err.Error())
		}
		log.Printf("%v: Packet received (readLen=%v)", conn.RemoteAddr(), readLen)
		packetSize := binary.LittleEndian.Uint16(buf)
		packetType := binary.LittleEndian.Uint16(buf[2:])
		log.Printf("  Size %v", packetSize)
		log.Printf("  Type %v", packetType)
		switch packetType {
		case convert.LPGPLWPBATTLERESULT:
			handleBattleResult(buf, rank, dbService)
		}
	}
	conn.Close()
	log.Printf("Conn closed %v", conn.RemoteAddr())
}

func handleBattleResult(buf []byte, rank *helpers.RankClient, dbService dbservice.Db) {
	log.Printf("BATTLERESULT received")
	// Parse
	bufReader := bytes.NewReader(buf)
	recvPacket, _ := convert.NewLwpBattleResult()
	err := binary.Read(bufReader, binary.LittleEndian, &recvPacket.S)
	if err != nil {
		log.Printf("binary.Read fail: %v", err.Error())
		return
	}
	player1 := convert.LwpBattleResultPlayer(&recvPacket.S, 0)
	player2 := convert.LwpBattleResultPlayer(&recvPacket.S, 1)
	id1 := convert.IdCuintToByteArray(player1.Id)
	id2 := convert.IdCuintToByteArray(player2.Id)
	var nickname1 string
	var nickname2 string
	convert.CCharArrayToGoString(&player1.Nickname, &nickname1)
	convert.CCharArrayToGoString(&player2.Nickname, &nickname2)
	winner := int(recvPacket.S.Winner)
	log.Printf("Battle result received; id1=%v, id2=%v, winner=%v", id1, id2, winner)
	backoff := 300 * time.Millisecond
	newScore1 := 0
	newScore2 := 0
	oldScore1 := rank.Get(backoff, id1)
	oldScore2 := rank.Get(backoff, id2)
	if oldScore1 != nil {
		newScore1 = oldScore1.Score + 1
		if winner == 1 {
			newScore1++
		}
	} else {
		log.Printf("rank.Get returned nil!")
		newScore1 = 1
	}
	if oldScore2 != nil {
		newScore2 = oldScore2.Score + 1
		if winner == 2 {
			newScore2++
		}
	} else {
		log.Printf("rank.Get returned nil!")
		newScore2 = 1
	}
	rank.Set(backoff, id1, newScore1, nickname1)
	rank.Set(backoff, id2, newScore2, nickname2)
	// update user db battle stat
	updateUserDbBattleStat(recvPacket, int(recvPacket.S.BattleTimeSec), int(recvPacket.S.Winner), dbService, 1)
	updateUserDbBattleStat(recvPacket, int(recvPacket.S.BattleTimeSec), int(recvPacket.S.Winner), dbService, 2)
}

func updateUserDbBattleStat(recvPacket *convert.BattleResult, battleTimeSec, winner int, dbService dbservice.Db, playerNo int) {
	player := convert.LwpBattleResultPlayer(&recvPacket.S, playerNo - 1)
	stat := convert.LwpBattleResultPlayerStat(&recvPacket.S, playerNo - 1)
	statOther := convert.LwpBattleResultPlayerStat(&recvPacket.S, playerNo % 2)
	totalHp := int(recvPacket.S.TotalHp)
	var leaseDb user.LeaseDb
	userId := convert.IdCuintToByteArray(player.Id)
	err := dbService.Lease(&userId, &leaseDb)
	if err != nil {
		log.Printf("lease failed with error: %v", err.Error())
	} else {
		//v := player.Stat.Hp
		bs := &leaseDb.Db.BattleStat
		bs.BattleTimeSec += battleTimeSec
		bs.Battle++
		if winner == 0 {
			bs.ConsecutiveVictory = 0
			bs.ConsecutiveDefeat = 0
		} else if winner == playerNo {
			bs.Victory++
			if int(stat.Hp) == totalHp && statOther.Hp == 0 {
				bs.PerfectVictory++
			}
			bs.ConsecutiveVictory++
			if bs.MaxConsecutiveVictory < bs.ConsecutiveVictory {
				bs.MaxConsecutiveVictory = bs.ConsecutiveVictory
			}
			bs.ConsecutiveDefeat = 0
		} else {
			bs.Defeat++
			if stat.Hp == 0 && int(statOther.Hp) == totalHp {
				bs.PerfectDefeat++
			}
			bs.ConsecutiveDefeat++
			if bs.MaxConsecutiveDefeat < bs.ConsecutiveDefeat {
				bs.MaxConsecutiveDefeat = bs.ConsecutiveDefeat
			}
			bs.ConsecutiveVictory = 0
		}
		bs.Dash += int(stat.Dash)
		maxPuckSpeed := int(stat.MaxPuckSpeed * 100)
		if bs.MaxPuckSpeed < maxPuckSpeed {
			bs.MaxPuckSpeed = maxPuckSpeed
		}
		bs.PuckTowerAttack += totalHp - int(statOther.Hp)
		bs.PuckTowerDamage += totalHp - int(stat.Hp)
		bs.PuckWallHit += int(stat.PuckWallHit)
		travelDistance := int(stat.TravelDistance * 100)
		bs.TravelDistance += travelDistance
		var reply int
		err = dbService.Write(&leaseDb, &reply)
		if err != nil {
			log.Printf("DB service write failed: %v", err.Error())
		}
	}
}
