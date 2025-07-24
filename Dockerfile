FROM alpine:latest

WORKDIR /app

RUN apk add --no-cache build-base ncurses-dev

COPY . /build

WORKDIR /build
RUN make
RUN mv quantum /app/
RUN make clean
WORKDIR /app
RUN rm -rf /build
RUN apk del build-base

EXPOSE 18467:18467

CMD ["/app/quantum", "-server"]


