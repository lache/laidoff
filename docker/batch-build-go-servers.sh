#!/bin/bash

rm -rf db
CC=/usr/local/musl/bin/musl-gcc go build --ldflags '-w -linkmode external -extldflags "-static"' -o ./db/db-server ../db-server/db.go

rm -rf match
CC=/usr/local/musl/bin/musl-gcc go build --ldflags '-w -linkmode external -extldflags "-static"' -o ./match/match-server ../match-server/match.go

rm -rf rank
CC=/usr/local/musl/bin/musl-gcc go build --ldflags '-w -linkmode external -extldflags "-static"' -o ./rank/rank-server ../rank-server/rank.go

rm -rf reward
CC=/usr/local/musl/bin/musl-gcc go build --ldflags '-w -linkmode external -extldflags "-static"' -o ./reward/reward-server ../reward-server/reward.go

rm -rf push
CC=/usr/local/musl/bin/musl-gcc go build --ldflags '-w -linkmode external -extldflags "-static"' -o ./push/push-server ../push-server/push.go

