#!/bin/bash
triplet="your-triplet"
vcpkg_path="PATH/TO/vcpkg"

cmake -B build -S. -DVCPKG_ROOT=$vcpkg_path -DVCPKG_TARGET_TRIPLET=$triplet

