#ifndef CONVERTER_IMAGE_GIF_H
#define CONVERTER_IMAGE_GIF_H

#include "IMAGE/ConverterImage.h"
#include <gif_lib.h>

class ConverterImage_GIF : public ConverterImage{
public:
    ConverterImage_GIF();
    ~ConverterImage_GIF();
    Image FileToImage(const std::vector<uint8_t>& data) override;
    Image FileToImage(const std::string& path_to_image) override;
    void ImageToFile(const Image& image_data, const std::string& path_to_output_image) override;
private:
    static void build_332_palette(GifColorType palette[256]);
    static unsigned char color_to_332_index(uint8_t r, uint8_t g, uint8_t b);
};



#endif
