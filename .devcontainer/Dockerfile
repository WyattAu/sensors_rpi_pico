FROM mcr.microsoft.com/devcontainers/base:ubuntu

# Install build tools and dependencies
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends \
        cmake \
        ninja-build \
        python3 \
        python3-pip \
        git \
        gcc-arm-none-eabi \
        libnewlib-arm-none-eabi \
        libstdc++-arm-none-eabi-newlib \
        build-essential \
        pkg-config \
    && apt-get autoremove -y && apt-get clean -y && rm -rf /var/lib/apt/lists/*

# Set Python 3 as default
RUN ln -s /usr/bin/python3 /usr/bin/python

# Create workspace directory
RUN mkdir -p /workspaces

# Set the default shell
ENV SHELL /bin/bash