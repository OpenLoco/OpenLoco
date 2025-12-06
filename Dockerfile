FROM ghcr.io/openloco/openloco:9-noble32

RUN apt-get update && apt-get install -y \
    gdb \
    pkg-config \
    libasound2-dev \
    libudev-dev \
    mesa-utils \
    vulkan-tools \
    libwayland-dev \
    libxkbcommon-dev \
    libvulkan1 \
    libvulkan-dev \
    libegl1-mesa-dev \
    libgles2-mesa-dev \
    libx11-dev \
    libxcursor-dev \
    libxrandr-dev \
    libxi-dev \
    libxcb1-dev \
    libxcb-icccm4-dev \
    libxcb-image0-dev \
    libxcb-keysyms1-dev \
    libxcb-randr0-dev \
    libxcb-shape0-dev \
    libxcb-xfixes0-dev \
    libxcb-xkb-dev \
    libgl1-mesa-dri \
    libglu1-mesa-dev \
    libglu1-mesa \
    libgtk-3-0 \
    alsa-utils \
    acl \
 && rm -rf /var/lib/apt/lists/*

ARG UID=1000
ARG GID=1000

# Set default shell
SHELL ["/bin/bash", "-c"]

# Create a non-root user (recommended for VS Code dev containers)
USER ubuntu

WORKDIR /openloco