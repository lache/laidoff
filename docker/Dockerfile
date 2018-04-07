FROM alpine:3.7
RUN apk add --no-cache bash
WORKDIR /app
ADD . /app
EXPOSE 19856
EXPOSE 10288/udp
CMD ["./runall.sh"]

