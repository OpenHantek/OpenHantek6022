#!/bin/bash

shopt -s globstar

rm -rf build
mkdir build && cd build && cmake ../ && make && cd ..

if [ ! -d ./hex ]; then
mkdir hex && cd hex && \
for MODEL in "2090" "2150" "2250" "5200" "5200A"; do
    wget http://www.hantek.com/Product/DSO2000/DSO${MODEL}_Driver.zip
    unzip DSO${MODEL}_Driver.zip
done && \
cd ..
fi

for f in hex/**/*.sys; do
    ./build/FirmwareExtractor $f && rm $f
done
