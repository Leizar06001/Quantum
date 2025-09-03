
<img src="https://github.com/Leizar06001/Quantum/blob/a6bbbd634c4e3ea9151a29a9b7f005ea2fc8b0f6/Quantum.png" width="500"/>

# Quantum

## Some sort of terminal chat

pull, make & launch with `./quantum`

if you want to run the server: `./quantum -server`
Default port is 18467, for now can only change it from 'includes.h' before compiling

You need ncurses, normaly the makefile takes care of it

## Docker usage

By default, the docker image will run `./quantum -server`, 
but you can use it as a client by typing the following:
```
docker run --rm -it -v "path/to/config.txt:/app/config.txt" --env TERM=xterm-256color ghcr.io/leizar06001/quantum sh -c "./quantum"
```

