
nohup socat TCP-LISTEN:6000,reuseaddr,fork UNIX-CLIENT:\"$DISPLAY\" &
echo $! > .socat.pid

nohup pulseaudio --load=module-native-protocol-tcp --exit-idle-time=-1 &
echo $! > .pulseaudio.pid

docker build . -t openloco && docker run --rm -it -e PULSE_SERVER=host.docker.internal -e DISPLAY=host.docker.internal:0 -v /tmp/.X11-unix:/tmp/.X11-unix -v $(pwd)/gamefiles:/openloco/gamefiles -v $HOME/.config/OpenLoco/save:/root/.config/OpenLoco/save -v ~/.config/pulse:/root/.config/pulse --cap-add=SYS_PTRACE --security-opt seccomp=unconfined openloco

# CLEANUPS!
kill -9 `cat .socat.pid`
rm .socat.pid
kill -9 `cat .pulseaudio.pid`
rm .pulseaudio.pid
