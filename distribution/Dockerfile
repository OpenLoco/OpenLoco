FROM ubuntu:18.04
RUN dpkg --add-architecture i386
RUN apt-get update
RUN apt-get -y upgrade
RUN apt-get install --no-install-recommends -y pkg-config:i386 gcc-multilib g++-multilib cmake git libsdl2-dev:i386 ninja-build
RUN apt-get install --no-install-recommends -y clang
