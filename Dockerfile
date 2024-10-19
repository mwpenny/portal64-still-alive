FROM ubuntu:24.04

WORKDIR /usr/src/app

ENV N64_LIBGCCDIR /opt/crashsdk/lib/gcc/mips64-elf/12.2.0
ENV PATH /opt/crashsdk/bin:$PATH
ENV PATH /root/.local/bin:$PATH
ENV ROOT /etc/n64

RUN apt-get update -y
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y \
    apt-utils 2>/dev/null
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y \
    ca-certificates
RUN echo "deb [trusted=yes] https://lambertjamesd.github.io/apt/ ./" | tee /etc/apt/sources.list.d/lambertjamesd.list && \
    echo "deb [trusted=yes] https://crashoveride95.github.io/apt/ ./" | tee /etc/apt/sources.list.d/n64sdk.list
RUN apt-get update -y && \ 
    dpkg --add-architecture i386 && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
    binutils-mips-n64 \
    build-essential \
    cimg-dev \
    cmake \
    ffmpeg \
    gcc-mips-n64 \
    git \
    imagemagick \
    libassimp-dev \
    libgl1 \
    liblua5.4-0 \
    liblua5.4-dev \
    libmpc-dev \
    libpng-dev \
    libtiff-dev \
    libxfixes3 \
    libxi6 \
    libxrender1 \
    libxxf86vm-dev \
    libyaml-cpp-dev \
    lua5.4 \
    makemask \
    newlib-mips-n64 \
    nodejs \
    n64sdk \
    pip \
    pipx \
    python3 \
    root-compatibility-environment \
    sfz2n64 \
    sox \
    unzip \
    vtf2png \
    wget

RUN mkdir /opt/blender
RUN wget -P /opt/blender https://download.blender.org/release/Blender3.6/blender-3.6.1-linux-x64.tar.xz

RUN tar -xf /opt/blender/blender-3.6.1-linux-x64.tar.xz -C /opt/blender
RUN rm /opt/blender/blender-3.6.1-linux-x64.tar.xz

ENV BLENDER_3_6 /opt/blender/blender-3.6.1-linux-x64/blender

RUN pipx ensurepath --force
RUN pipx install vpk

# Avoid "dubious ownership" error when running git commands
RUN git config --global --add safe.directory "$PWD"