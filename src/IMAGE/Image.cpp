#include "Image.h"
#include <iostream>

void Image::Print(){
    std::printf("[IMAGE]\n\t[width]: [%i]\n\t[height]: [%i]\n\t[count_pixels]: [%i]\n",width,height,(int)pixels.size());
}
