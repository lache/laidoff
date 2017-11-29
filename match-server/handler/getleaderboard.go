package handler

import (
	"net"
	"log"
	"time"
	"../service"
	"../convert"
)

func HandleGetLeaderboard(buf []byte, conn net.Conn, serviceList *service.ServiceList) {
	log.Printf("GETLEADERBOARD received")
	// Parse
	recvPacket, err := convert.ParseGetLeaderboard(buf)
	if err != nil {
		log.Printf("HandleGetLeaderboard fail: %v", err.Error())
	}
	startIndex := int(recvPacket.Start_index)
	count := int(recvPacket.Count)
	leaderboardReply := serviceList.Rank.GetLeaderboard(300*time.Millisecond, startIndex, count)
	reply := convert.NewLeaderboard(leaderboardReply)
	replyBuf := convert.Packet2Buf(reply)
	conn.Write(replyBuf)
}
