#define _CRT_SECURE_NO_WARNINGS
#include "ConverterImage.JPG.h"
#include <stb_image.h>
#include <stb_image_write.h>
#include <iostream> 
#include <vector>

ConverterImage_JPG::ConverterImage_JPG() : ConverterImage("jpg"){

}

Image ConverterImage_JPG::FileToImage(const std::vector<uint8_t>& data_file){
    int w = 0, h = 0, origChannels = 0;

    unsigned char* data = stbi_load_from_memory(
        data_file.data(),
        data_file.size(),
        &w,
        &h,
        &origChannels, 4);

    Image img;
    img.width = w;
    img.height = h;
    img.channels = 4;
    img.pixels.resize(static_cast<size_t>(w) * static_cast<size_t>(h));

    const size_t pixelCount = static_cast<size_t>(w) * static_cast<size_t>(h);
    for (size_t i = 0; i < pixelCount; ++i) {
        size_t base = i * 4;
        img.pixels[i].r = data[base + 0];
        img.pixels[i].g = data[base + 1];
        img.pixels[i].b = data[base + 2];
        img.pixels[i].a = data[base + 3];
    }

    stbi_image_free(data);
    return img;
}

Image ConverterImage_JPG::FileToImage(const std::string& path_to_image){
    int w = 0, h = 0, origChannels = 0;
    unsigned char* data = stbi_load(path_to_image.c_str(), &w, &h, &origChannels, 4);
    Image img;
    img.width = w;
    img.height = h;
    img.channels = 4;
    img.pixels.resize(static_cast<size_t>(w) * static_cast<size_t>(h));

    const size_t pixelCount = static_cast<size_t>(w) * static_cast<size_t>(h);
    for (size_t i = 0; i < pixelCount; ++i) {
        size_t base = i * 4;
        img.pixels[i].r = data[base + 0];
        img.pixels[i].g = data[base + 1];
        img.pixels[i].b = data[base + 2];
        img.pixels[i].a = data[base + 3];
    }

    stbi_image_free(data);
    return img;
}

void ConverterImage_JPG::ImageToFile(const Image& image_data, const std::string& path_to_output_image){

    std::vector<uint8_t> pixels(image_data.width * image_data.height * 4);

    for(int x = 0; x < image_data.width; x++){
        for(int y = 0; y < image_data.height; y++){
            int idx = (y * image_data.width + x);
            const Pixel& p = image_data.pixels[idx];

            pixels[idx*4 + 0] = p.r;
            pixels[idx*4 + 1] = p.g;
            pixels[idx*4 + 2] = p.b;
            pixels[idx*4 + 3] = p.a;

        }
    }

    if (!stbi_write_jpg(path_to_output_image.c_str(), image_data.width, image_data.height, 4, pixels.data(), 100)) {
        std::cerr << "Failed to write JPG to " << path_to_output_image << std::endl;
        exit(4);
    }
}