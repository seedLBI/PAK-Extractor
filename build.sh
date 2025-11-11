#!/bin/bash
triplet="arm64-linux"
vcpkg_path="/home/seedlbi/vcpkg"

cmake -B build -S. -DVCPKG_ROOT=$vcpkg_path -DVCPKG_TARGET_TRIPLET=$triplet

if [ $? -eq 0 ]; then
    cd build
    make
fi
