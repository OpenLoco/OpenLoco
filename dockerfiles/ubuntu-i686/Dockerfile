FROM ubuntu:18.04

RUN dpkg --add-architecture i386 \
    && apt-get update \
    && apt-get -y upgrade \
    && apt-get install --no-install-recommends -y
        clang \
        cmake \
        g++-multilib \
        gcc-multilib \
        git \
        libopenal-dev:i386 \
        libpng-dev:i386 \
        libsdl2-dev:i386 \
        libsdl2-mixer-dev:i386 \
        libyaml-cpp-dev:i386 \
        ninja-build \
        pkg-config:i386 \
    && rm -rf /var/lib/apt/lists/*
