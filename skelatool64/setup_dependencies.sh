#!/usr/bin/bash

sudo apt install -y libpng-dev libtiff-dev libassimp-dev

git clone https://github.com/jbeder/yaml-cpp.git

cmake -S yaml-cpp -B yaml-cpp
make -C yaml-cpp

wget http://cimg.eu/files/CImg_latest.zip
unzip CImg_latest.zip
mv CImg-3.1.3_pre051622 cimg