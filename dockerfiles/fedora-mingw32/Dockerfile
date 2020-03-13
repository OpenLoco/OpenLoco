FROM fedora:31
RUN dnf update -y && \
  dnf install -y mingw32-boost mingw32-boost-static mingw32-SDL2 mingw32-SDL2-static mingw32-SDL2_mixer cmake git ninja-build dnf-plugins-core mingw32-yaml-cpp mingw32-libpng && \
  dnf clean all --enablerepo=\*
