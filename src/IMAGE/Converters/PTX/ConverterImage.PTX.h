#ifndef CONVERTER_IMAGE_PTX_H
#define CONVERTER_IMAGE_PTX_H

#include "IMAGE/ConverterImage.h"
#include <iostream>
#include <vector>

class ConverterImage_PTX : public ConverterImage{
public:
    ConverterImage_PTX();
    Image FileToImage(const std::vector<uint8_t>& data) override;
    Image FileToImage(const std::string& path_to_image) override;
    void ImageToFile(const Image& image_data, const std::string& path_to_output_image) override;
private:
    static uint32_t read_u32_le(const uint8_t* p);
};

#endif
