#ifndef CONVERTER_IMAGE_JPG_H
#define CONVERTER_IMAGE_JPG_H

#include "IMAGE/ConverterImage.h"
#include <iostream>
#include <vector>

class ConverterImage_JPG : public ConverterImage{
public:
    ConverterImage_JPG();
    Image FileToImage(const std::vector<uint8_t>& data) override;
    Image FileToImage(const std::string& path_to_image) override;
    void ImageToFile(const Image& image_data, const std::string& path_to_output_image) override;
};

#endif