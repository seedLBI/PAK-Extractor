#ifndef CONVERTER_IMAGE_H
#define CONVERTER_IMAGE_H

#include <cstdint>
#include <string>
#include <vector>
#include "Image.h"

class ConverterImage {
public:
    ConverterImage(const std::string& extensionName);
    std::string GetExtension();
    virtual Image FileToImage(const std::vector<uint8_t>& data);
    virtual Image FileToImage(const std::string& path_to_image);
    virtual void ImageToFile(const Image& image_data, const std::string& path_to_output_image);

private:
    const std::string extension;
};

#endif
