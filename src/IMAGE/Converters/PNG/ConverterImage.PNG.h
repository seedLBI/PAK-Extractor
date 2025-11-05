#ifndef CONVERTER_IMAGE_PNG_H
#define CONVERTER_IMAGE_PNG_H

#include "IMAGE/ConverterImage.h"
#include <iostream>
#include <vector>

class ConverterImage_PNG : public ConverterImage{
public:
    ConverterImage_PNG();
    Image FileToImage(const std::vector<uint8_t>& data) override;
    Image FileToImage(const std::string& path_to_image) override;
    void ImageToFile(const Image& image_data, const std::string& path_to_output_image) override;

};

#endif
