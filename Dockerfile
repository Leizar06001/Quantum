FROM alpine:latest

WORKDIR /app

RUN apk update && apk add gcc build-base ncurses-dev

COPY . .

RUN make

EXPOSE 18467

CMD ["/app/quantum", "-server"]


