FROM alpine:latest

COPY . /build
WORKDIR /build
RUN mkdir /app
RUN apk add --no-cache build-base ncurses-dev \
    && make \
    && mv quantum /app/ \
    && make clean \
    && apk del build-base \
    && rm -rf /build
WORKDIR /app

EXPOSE 18467:18467

CMD ["/app/quantum", "-server"]


