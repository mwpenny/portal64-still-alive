from ubuntu:23.04

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
    pipx \
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
    mpg123 \
    ffmpeg \
    wget

RUN mkdir /opt/blender
RUN wget -P /opt/blender https://download.blender.org/release/Blender3.6/blender-3.6.1-linux-x64.tar.xz

RUN tar -xf /opt/blender/blender-3.6.1-linux-x64.tar.xz -C /opt/blender
RUN rm /opt/blender/blender-3.6.1-linux-x64.tar.xz

ENV BLENDER_3_6 /opt/blender/blender-3.6.1-linux-x64/blender

RUN pipx ensurepath --force
RUN pipx install vpk
