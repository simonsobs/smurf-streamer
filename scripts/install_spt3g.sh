#!/usr/bin/env bash

git clone https://github.com/CMB-S4/spt3g_software.git

cd spt3g_software
mkdir -p build
cd build
cmake .. -DPYTHON_EXECUTABLE=`which python3`
make
