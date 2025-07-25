# Quantum

## Some sort of terminal chat

pull, make & launch with `./quantum <username> <server_ip>`

if you want to run the server: `./quantum -server`

You need ncurses, normaly the makefile takes care of it

## Docker usage

By default, the docker image will run `./quantum -server`, 
but you can use it as a client by typing the following:
```
docker run --rm -it -v "path/to/config.txt:/app/config.txt" --env TERM=xterm-256color ghcr.io/leizar06001/quantum sh -c "./quantum <username> <server_ip>"
```

