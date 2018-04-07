#!/bin/bash

# laidoff-server (battle-server)
rm -rf battle
mkdir battle
rm -rf build-server
mkdir build-server
cd build-server
CXXFLAGS=-static cmake ../.. -DSERVER_ONLY=1
make laidoff-server
cp bin/laidoff-server ../battle
cd ..

# db-server
rm -rf db
CC=/usr/local/musl/bin/musl-gcc go build --ldflags '-w -linkmode external -extldflags "-static"' -o ./db/db-server ../db-server/db.go
# db-server resources
cp ../db-server/*.txt db
cp ../db-server/*.html db

# match-server
rm -rf match
CC=/usr/local/musl/bin/musl-gcc go build --ldflags '-w -linkmode external -extldflags "-static"' -o ./match/match-server ../match-server/match.go
# match-server resources
cp ../match-server/conf.json.template match/conf.json

# rank-server
rm -rf rank
CC=/usr/local/musl/bin/musl-gcc go build --ldflags '-w -linkmode external -extldflags "-static"' -o ./rank/rank-server ../rank-server/rank.go
# rank-server resources
# ---

# reward-server
rm -rf reward
CC=/usr/local/musl/bin/musl-gcc go build --ldflags '-w -linkmode external -extldflags "-static"' -o ./reward/reward-server ../reward-server/reward.go
# reward-server resources
# ---

# push-server
rm -rf push
CC=/usr/local/musl/bin/musl-gcc go build --ldflags '-w -linkmode external -extldflags "-static"' -o ./push/push-server ../push-server/push.go
# push-server resources
cp -r ../push-server/cert push
cp ../push-server/*.html push
