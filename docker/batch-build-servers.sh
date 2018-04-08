#!/bin/bash

rm -rf bin
rm -rf services

mkdir bin
mkdir services

# laidoff-server (battle-server)
mkdir build-server
cd build-server
CXXFLAGS=-static cmake ../.. -DSERVER_ONLY=1
make -j8 laidoff-server
cp bin/laidoff-server ../bin/
cd ..

# br-server (all-in-one server binary)
CC=/usr/local/musl/bin/musl-gcc go build --ldflags '-w -linkmode external -extldflags "-static"' -o ./bin/br-server ../br-server/br.go

cd services

# laidoff-server
mkdir battle

# db-server
mkdir db
# db-server resources
cp ../../db-server/*.txt db
cp ../../db-server/*.html db

# match-server
mkdir match
# match-server resources
cp ../../match-server/conf.json.template match/conf.json

# rank-server
mkdir rank
# rank-server resources
# ---

# reward-server
mkdir reward
# reward-server resources
# ---

# push-server
mkdir push
# push-server resources
cp ../../push-server/*.html push

cd ..

