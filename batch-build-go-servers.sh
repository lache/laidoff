#!/bin/bash

CC=/usr/local/musl/bin/musl-gcc go build --ldflags '-w -linkmode external -extldflags "-static"' -o ./go-bin/db-server ./db-server/db.go
CC=/usr/local/musl/bin/musl-gcc go build --ldflags '-w -linkmode external -extldflags "-static"' -o ./go-bin/match-server ./match-server/match.go
CC=/usr/local/musl/bin/musl-gcc go build --ldflags '-w -linkmode external -extldflags "-static"' -o ./go-bin/rank-server ./rank-server/rank.go
CC=/usr/local/musl/bin/musl-gcc go build --ldflags '-w -linkmode external -extldflags "-static"' -o ./go-bin/reward-server ./reward-server/reward.go
CC=/usr/local/musl/bin/musl-gcc go build --ldflags '-w -linkmode external -extldflags "-static"' -o ./go-bin/push-server ./push-server/push.go

