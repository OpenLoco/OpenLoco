FROM ubuntu:18.04

WORKDIR /openloco

RUN dpkg --add-architecture i386 && \
    apt-get -y update && \
    apt-get install --no-install-recommends -y apt-utils pkg-config:i386 libsdl2-dev:i386 libsdl2-mixer-dev:i386 libpng-dev:i386 libyaml-cpp-dev:i386 libc6:i386 libncurses5:i386 libstdc++6:i386 \
    libmikmod-dev:i386 libfishsound1-dev:i386 libsmpeg-dev:i386 liboggz2-dev:i386 libflac-dev:i386 libfluidsynth-dev:i386 libsdl2-mixer-dev:i386 libsdl2-mixer-2.0-0:i386 \
    libsdl2-dev:i386 libsdl2-2.0-0:i386 \
    libpng16-16:i386 \
    pulseaudio-utils alsa-utils libasound2 libasound2-plugins pulseaudio pulseaudio-utils \ 
    pkg-config:i386 gcc-multilib g++-multilib git libsdl2-dev:i386 libsdl2-mixer-dev:i386 ninja-build libpng-dev:i386 libyaml-cpp-dev:i386 \
    clang cmake \
    pkg-config:i386 \
    gcc-multilib \
    g++-multilib \
    cmake \
    git \
    ninja-build \
    libsdl2-dev:i386 \
    libsdl2-mixer-dev:i386 \
    libpng-dev:i386 \
    libyaml-cpp-dev:i386

COPY . /openloco

WORKDIR /openloco/build

RUN cmake .. -G Ninja \
    -DCMAKE_BUILD_TYPE=release \
    -DOPENLOCO_USE_CCACHE=OFF \
    -DSDL2_DIR=/usr/lib/i386-linux-gnu/cmake/SDL2 \
    -DSDL2_MIXER_PATH=/usr/lib/i386-linux-gnu \
    -Dyaml-cpp_DIR=/usr/lib/i386-linux-gnu/cmake/yaml-cpp \
    -DPNG_LIBRARY=/usr/lib/i386-linux-gnu/libpng16.so \
    -DPNG_PNG_INCLUDE_DIR=/usr/include \
    -DZLIB_LIBRARY=/usr/lib/i386-linux-gnu/libz.so && \
    ninja -j4

COPY data /openloco/build/data
RUN chmod 777 /openloco/build/openloco

VOLUME /openloco/gamefiles

RUN mkdir -p $HOME/.config/OpenLoco && mkdir -p /root/.config/OpenLoco/save/ && \
    touch $HOME/.config/OpenLoco/openloco.yml && \
    echo 'loco_install_path: /openloco/gamefiles' > $HOME/.config/OpenLoco/openloco.yml && \
    chmod 777 -R $HOME/.config/OpenLoco

ENTRYPOINT [ "/openloco/build/openloco" ]
CMD [ "/openloco/build/openloco" ]