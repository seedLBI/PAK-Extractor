#ifndef SCALING_IMAGE_H
#define SCALING_IMAGE_H

#include "Image.h"
#include <cmath>

Image IntegerScaleImage(const Image& input, const int& scale);

Image BilinearScaleImage(const Image& input, const int& scale);

Image BicubicScaleImage(const Image& input, const int& scale);

#endif
