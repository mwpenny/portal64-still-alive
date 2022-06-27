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
    libnustd \
    makemask \
    root-compatibility-environment \
    build-essential \
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
    mpg123

COPY skelatool64/src skelatool64/src
COPY skelatool64/main.cpp skelatool64/main.cpp
COPY skelatool64/makefile skelatool64/makefile

RUN git clone https://github.com/jbeder/yaml-cpp.git skelatool64/yaml-cpp

RUN cmake -S skelatool64/yaml-cpp -B skelatool64/yaml-cpp
RUN make -C skelatool64/yaml-cpp

RUN wget http://cimg.eu/files/CImg_latest.zip
RUN unzip CImg_latest.zip
RUN mv CImg-3.1.3_pre051622 skelatool64/cimg

RUN make -C skelatool64

RUN pip install vpk

COPY Makefile Makefile
COPY tools/export_fbx.py tools/export_fbx.py
COPY tools/generate_level_list.js tools/generate_level_list.js
COPY tools/generate_sound_ids.js tools/generate_sound_ids.js
COPY asm asm
COPY assets assets
COPY src src
COPY portal.ld portal.ld

CMD make