#!/bin/bash

# laidoff-server (battle-server)
rm -rf battle
mkdir battle
rm -rf build-server
mkdir build-server
cd build-server
CXXFLAGS=-static cmake ../.. -DSERVER_ONLY=1
make -j8 laidoff-server
cp bin/laidoff-server ../battle
cd ..

# br-server (all-in-one server binary)
rm -rf br
CC=/usr/local/musl/bin/musl-gcc go build --ldflags '-w -linkmode external -extldflags "-static"' -o ./br/br-server ../br-server/br.go

# db-server
rm -rf db
mkdir db
# db-server resources
cp ../db-server/*.txt db
cp ../db-server/*.html db

# match-server
rm -rf match
mkdir match
# match-server resources
cp ../match-server/conf.json.template match/conf.json

# rank-server
rm -rf rank
mkdir rank
# rank-server resources
# ---

# reward-server
rm -rf reward
mkdir reward
# reward-server resources
# ---

# push-server
rm -rf push
mkdir push
# push-server resources
cp ../push-server/*.html push

