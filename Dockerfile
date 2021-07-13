FROM ubuntu:18.04 AS build

WORKDIR /openloco/build

RUN dpkg --add-architecture i386
RUN apt-get -y update
RUN apt-get install --no-install-recommends -y pkg-config:i386 \
    libsdl2-dev:i386 \
    libsdl2-mixer-dev:i386 \
    libpng-dev:i386 \
    libyaml-cpp-dev:i386 \
    libc6:i386 \
    libncurses5:i386 \
    libstdc++6:i386 \
    gcc-multilib \
    g++-multilib \
    clang \
    cmake \
    ninja-build

COPY . /openloco

RUN CXXFLAGS="-m32" cmake .. -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=develop && ninja -k0

FROM ubuntu:18.04

RUN dpkg --add-architecture i386
RUN apt-get -y update
RUN apt-get -y install libsdl2-2.0-0:i386 \
    libsdl2-mixer-2.0-0:i386 \
    libc6:i386 \
    libncurses5:i386 \
    libstdc++6:i386 \
    libpng-dev:i386 \
    libyaml-cpp-dev:i386

COPY data /openloco/build/data
COPY openloco.yml /root/.config/OpenLoco/openloco.yml
COPY --from=build /openloco/build /openloco/build
VOLUME /openloco/gamefiles

CMD ["/openloco/build/openloco"]