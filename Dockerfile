from node:slim

WORKDIR /usr/src/app

ENV N64_LIBGCCDIR /opt/crashsdk/lib/gcc/mips64-elf/11.2.0
ENV PATH /opt/crashsdk/bin:$PATH
ENV ROOT /etc/n64

RUN apt update -y
RUN apt install -y ca-certificates

RUN echo "deb [trusted=yes] https://lambertjamesd.github.io/apt/ ./" | tee /etc/apt/sources.list.d/lambertjamesd.list
RUN echo "deb [trusted=yes] https://crashoveride95.github.io/apt/ ./" | tee /etc/apt/sources.list.d/n64sdk.list
RUN apt update -y

RUN dpkg --add-architecture i386

RUN apt install -y binutils-mips-n64 \
    gcc-mips-n64 \
    n64sdk \
    makemask \
    root-compatibility-environment \
    build-essential \
    libmpc-dev \
    vtf2png \
    skeletool64 \
    libxi6 \
    libxxf86vm-dev \
    libxfixes3 \
    libxrender1 \
    libgl1 \
    python3 \
    pip \
    imagemagick

RUN pip install vpk

COPY Makefile Makefile
COPY tools/export_fbx.py tools/export_fbx.py
COPY tools/generate_level_list.js tools/generate_level_list.js
COPY asm asm
COPY assets assets
COPY src src
copy portal.ld portal.ld

CMD make