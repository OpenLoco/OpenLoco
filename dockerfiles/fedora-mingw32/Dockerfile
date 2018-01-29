FROM fedora:27
RUN dnf update -y && \
  dnf install -y mingw32-boost mingw32-boost-static mingw32-SDL2 mingw32-SDL2-static cmake git ninja-build && \
  dnf clean all --enablerepo=\*
