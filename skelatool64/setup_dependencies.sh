#!/usr/bin/bash

sudo apt install -y libpng-dev libtiff-dev libassimp-dev g++ liblua5.4-dev cmake

pushd $(dirname "$0")

git clone https://github.com/jbeder/yaml-cpp.git

cmake -S yaml-cpp -B yaml-cpp
make -C yaml-cpp

wget -O CImg_3.1.3.zip http://cimg.eu/files/CImg_3.1.3.zip
unzip -o CImg_3.1.3.zip
rm -rf cimg
mv CImg-3.1.3 cimg

popd
