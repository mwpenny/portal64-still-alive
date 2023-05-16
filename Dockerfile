from ubuntu:22.04

WORKDIR /usr/src/app

ENV N64_LIBGCCDIR /opt/crashsdk/lib/gcc/mips64-elf/12.2.0
ENV PATH /opt/crashsdk/bin:$PATH
ENV ROOT /etc/n64
ENV BLENDER_3_0 /usr/bin/blender

RUN apt update -y && \
    DEBIAN_FRONTEND=noninteractive apt install -y \
    ca-certificates
RUN echo "deb [trusted=yes] https://lambertjamesd.github.io/apt/ ./" | tee /etc/apt/sources.list.d/lambertjamesd.list && \
    echo "deb [trusted=yes] https://crashoveride95.github.io/apt/ ./" | tee /etc/apt/sources.list.d/n64sdk.list
RUN apt update -y && \ 
    dpkg --add-architecture i386 && \
    DEBIAN_FRONTEND=noninteractive apt install -y \
    binutils-mips-n64 \
    gcc-mips-n64 \
    n64sdk \
    libnustd \
    makemask \
    root-compatibility-environment \
    libmpc-dev \
    sfz2n64 \
    vtf2png \
    libxi6 \
    libxxf86vm-dev \
    libxfixes3 \
    libxrender1 \
    libgl1 \
    python3 \
    pip \
    imagemagick \
    libpng-dev \
    libtiff-dev \
    libassimp-dev \
    git \
    cmake \
    build-essential \
    wget \
    unzip \
    sox \
    nodejs \
    lua5.4 \
    liblua5.4-dev \
    liblua5.4-0 \
    blender \
    mpg123

RUN pip install vpk
