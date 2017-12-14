package service

type base int

const (
	Score base = iota
	CumulativeVictory
	CumulativeDefeat
	ConsecutiveVictory
	ConsecutiveDefeat

	Count
)
