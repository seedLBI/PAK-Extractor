#ifndef IMAGE_H
#define IMAGE_H

#include <cstdint>
#include <vector>

struct Pixel{
    uint8_t r,g,b,a;
};

struct Image{
    int width, height, channels;
    std::vector<Pixel> pixels;
    void Print();
};

#endif

