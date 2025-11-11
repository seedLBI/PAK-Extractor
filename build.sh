#!/bin/bash
triplet="your-triplet"
vcpkg_path="/PATH/TO/VCPKG"

cmake -B build -S. -DVCPKG_ROOT=$vcpkg_path -DVCPKG_TARGET_TRIPLET=$triplet

if [ $? -eq 0 ]; then
    cd build
    make
fi
