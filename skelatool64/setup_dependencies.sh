#!/usr/bin/bash

sudo apt install -y libpng-dev libtiff-dev libassimp-dev g++ liblua5.4-dev cmake

pushd $(dirname "$0")

git clone https://github.com/jbeder/yaml-cpp.git

cmake -S yaml-cpp -B yaml-cpp
make -C yaml-cpp

wget http://cimg.eu/files/CImg_3.1.3.zip
unzip CImg_3.1.3.zip
mv CImg-3.1.3 cimg

popd
