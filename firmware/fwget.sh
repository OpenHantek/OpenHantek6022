#!/bin/bash

shopt -s globstar

cd "$(dirname "$0")"
rm -rf tmp
mkdir tmp

cd tmp
for MODEL in "2090" "2150" "2250" "5200" "5200A"; do
    wget http://www.hantek.com/Product/DSO2000/DSO${MODEL}_Driver.zip
    unzip DSO${MODEL}_Driver.zip
done
cd ..

for f in tmp/**/*.sys; do
    extractfw/extractfw $f
done
mv tmp/**/*.hex .
rm -rf tmp
