FROM --platform=linux/arm64 dtcooper/raspberrypi-os:bookworm

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libcamera-dev \
    libevent-dev \
    libusb-1.0-0-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

RUN git config --global --add safe.directory /workspace

WORKDIR /workspace
CMD ["/bin/bash"]