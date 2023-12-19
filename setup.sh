#!/bin/bash

# Add repositories to system sources
echo "deb [trusted=yes] https://crashoveride95.github.io/apt/ ./" | sudo tee /etc/apt/sources.list.d/n64sdk.list
echo "deb [trusted=yes] https://lambertjamesd.github.io/apt/ ./" | sudo tee /etc/apt/sources.list.d/lambertjamesd.list

# Update system package lists
sudo dpkg --add-architecture i386
sudo apt update

# Install various packages
sudo apt install binutils-mips-n64 gcc-mips-n64 git imagemagick liblua5.4-0 liblua5.4-dev libnustd lua5.4 makemask mpg123 newlib-mips-n64 nodejs n64sdk root-compatibility-environment sfz2n64 sox unzip vtf2png -y

# Install pipx packages
sudo apt install pipx -y
pipx ensurepath --force
pipx install vpk

# Install Blender and FFmpeg  specific versions via snap
sudo snap install blender --channel=3.6lts/stable --classic
sudo snap install ffmpeg

# Append environment variables to .bashrc
echo 'export N64_LIBGCCDIR=/opt/crashsdk/lib/gcc/mips64-elf/12.2.0' >> ~/.bashrc
echo 'export BLENDER_3_6=/snap/bin/blender' >> ~/.bashrc
echo 'export PATH=$PATH:/opt/crashsdk/bin' >> ~/.bashrc
echo 'export ROOT=/etc/n64' >> ~/.bashrc

# Source the updated .bashrc to apply changes in the current terminal
source ~/.bashrc

echo "Setup is almost complete. Add the files from the Portal folder to portal64/vpk"
read -p "When complete, press Enter to finish setup."

# Displaying 'Setup complete' message after user input
echo "Setup complete."
