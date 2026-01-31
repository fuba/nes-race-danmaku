FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install cc65 toolchain and build dependencies
RUN apt-get update && apt-get install -y \
    cc65 \
    make \
    git \
    python3 \
    python3-pip \
    wget \
    curl \
    xxd \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Verify cc65 installation
RUN cc65 --version && ca65 --version && ld65 --version

# Clone neslib and nesdoug libraries
RUN git clone --depth 1 https://github.com/nesdoug/01_Hello.git /tmp/nesdoug && \
    mkdir -p /opt/neslib && \
    cp /tmp/nesdoug/*.s /opt/neslib/ 2>/dev/null || true && \
    cp /tmp/nesdoug/*.h /opt/neslib/ 2>/dev/null || true && \
    cp /tmp/nesdoug/*.cfg /opt/neslib/ 2>/dev/null || true && \
    rm -rf /tmp/nesdoug

# Clone cc65-nes-examples for reference
RUN git clone --depth 1 https://github.com/jmk/cc65-nes-examples.git /opt/cc65-nes-examples

# Set include paths
ENV CC65_INC=/opt/neslib:/opt/cc65-nes-examples/src

# Default command
CMD ["make"]
