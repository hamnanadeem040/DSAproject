#!/bin/bash
echo "Building FileZip v2.0..."
g++ -std=c++17 -O2 src/main_cli.cpp src/huffman.cpp src/lzw.cpp src/rle.cpp \
    -I include -o filezip && echo "Done! Run: ./filezip demo"
