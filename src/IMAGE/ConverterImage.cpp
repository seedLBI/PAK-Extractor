#include "ConverterImage.h"

ConverterImage::ConverterImage(const std::string& extensionName) : extension(extensionName){

}

Image ConverterImage::FileToImage(const std::vector<uint8_t>& data){
    return {};
}

Image ConverterImage::FileToImage(const std::string& path_to_image){
    return {};
}

void ConverterImage::ImageToFile(const Image& image_data, const std::string& path_to_output_image) {

}
std::string ConverterImage::GetExtension(){
    return extension;
}
