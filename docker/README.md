### How to build docker image ###

`docker build -t br .`

### How to run docker a new container ###

`docker run -d -p 19856:19856 -p 10288:10288/udp -e "BATTLE_HOST=<public IP>" br`

