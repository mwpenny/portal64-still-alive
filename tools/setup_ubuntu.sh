#!/bin/bash

# Add repositories to system sources
echo "deb [trusted=yes] https://crashoveride95.github.io/apt/ ./" | sudo tee /etc/apt/sources.list.d/n64sdk.list
echo "deb [trusted=yes] https://lambertjamesd.github.io/apt/ ./" | sudo tee /etc/apt/sources.list.d/lambertjamesd.list

# Update system package lists
sudo dpkg --add-architecture i386
sudo apt update

# Install various packages
sudo apt install -y \
    binutils-mips-n64 \
    build-essential \
    cimg-dev \
    cmake \
    ffmpeg \
    gcc-mips-n64 \
    git \
    imagemagick \
    libassimp-dev \
    liblua5.4-0 \
    liblua5.4-dev \
    libpng-dev \
    libtiff-dev \
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
    vtf2png

# Ubuntu WSL needs these for some reason.
sudo apt-get update && sudo apt-get install libxfixes3 libxi6 libxkbcommon0 libxxf86vm1 libgl1-mesa-glx -y

# Install Blender specific version via snap
sudo snap install blender --channel=3.6lts/stable --classic

# Append environment variables to .bashrc
echo 'export PATH=$PATH:/opt/crashsdk/bin' >> ~/.bashrc
echo 'export ROOT=/etc/n64' >> ~/.bashrc

# Install pipx packages
sudo apt install pipx -y
pipx ensurepath --force
pipx install vpk

# Source the updated .bashrc to apply changes in the current terminal
source ~/.bashrc

echo "Setup complete. Please restart the terminal if paths are not updated."
