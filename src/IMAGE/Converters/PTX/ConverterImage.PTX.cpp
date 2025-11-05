#include "ConverterImage.PTX.h"

#include <fstream>
#include <squish.h>


uint32_t ConverterImage_PTX::read_u32_le(const uint8_t* p){
        return uint32_t(p[0]) | (uint32_t(p[1])<<8) | (uint32_t(p[2])<<16) | (uint32_t(p[3])<<24);
}

ConverterImage_PTX::ConverterImage_PTX() : ConverterImage("ptx"){

}

Image ConverterImage_PTX::FileToImage(const std::string& path_to_image){
    std::ifstream ifs(path_to_image, std::ios::binary | std::ios::ate);
    if(!ifs) throw std::runtime_error("Failed to open " + path_to_image);
    std::streamsize size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    std::vector<uint8_t> buf(size);
    if(!ifs.read(reinterpret_cast<char*>(buf.data()), size)) throw std::runtime_error("Failed to read file");
    return FileToImage(buf);
}

Image ConverterImage_PTX::FileToImage(const std::vector<uint8_t>& data){
    // Expecting a DDS blob (many PTX variants embed a DDS) - this implementation parses DDS.
    if(data.size() < 128) throw std::runtime_error("File too small to be a DDS/PTX.");

    // Check "DDS " magic
    if(!(data[0] == 'D' && data[1] == 'D' && data[2] == 'S' && data[3] == ' ')){
        throw std::runtime_error("Not a DDS/PTX that this converter recognizes (missing 'DDS ' magic).");
    }

    const uint8_t* ptr = data.data();
    uint32_t dwSize = read_u32_le(ptr + 4); // should be 124
    if(dwSize != 124) {
        // still proceed but warn
        std::cerr << "Warning: DDS header size != 124 (got " << dwSize << "). Proceeding cautiously.\n";
    }

    uint32_t height = read_u32_le(ptr + 12);
    uint32_t width  = read_u32_le(ptr + 16);
    // pixel format fourCC is at offset 84
    const uint8_t* pf = ptr + 76; // DDS_PIXELFORMAT starts at offset 76 from file start (4 + 76 = 80?) but fourCC at 84 from file start
    // More reliably:
    const uint8_t* fourcc_ptr = ptr + 84;
    std::string fourcc(reinterpret_cast<const char*>(fourcc_ptr), 4);

    // Data offset: DDS magic (4) + header (dwSize, normally 124) = 128
    size_t data_offset = 4 + dwSize;
    if(data.size() <= data_offset) throw std::runtime_error("DDS has no image data.");

    Image out;
    out.width = static_cast<int>(width);
    out.height = static_cast<int>(height);
    out.channels = 4;
    out.pixels.resize(size_t(width) * size_t(height));

    // Decide format and decompress if needed
    if(fourcc == "DXT1" || fourcc == "DXT3" || fourcc == "DXT5") {
        int flags = 0;
        if(fourcc == "DXT1") flags = squish::kDxt1;
        else if(fourcc == "DXT3") flags = squish::kDxt3;
        else if(fourcc == "DXT5") flags = squish::kDxt5;
        // optionally add quality flag:
        flags |= squish::kColourClusterFit; // good quality; optionally kColourRangeFit for speed

        // Allocate RGBA buffer
        std::vector<uint8_t> rgba(size_t(width) * size_t(height) * 4);

        const uint8_t* compressed = data.data() + data_offset;
        // libsquish expects blocks pointer and will decompress into RGBA
        squish::DecompressImage(rgba.data(), int(width), int(height), compressed, flags);

        // copy to Pixel array
        size_t idx = 0;
        for(size_t y=0;y<size_t(height);++y){
            for(size_t x=0;x<size_t(width);++x){
                Pixel px;
                px.r = rgba[idx+0];
                px.g = rgba[idx+1];
                px.b = rgba[idx+2];
                px.a = rgba[idx+3];
                out.pixels[y*size_t(width) + x] = px;
                idx += 4;
            }
        }
        return out;
    }
    else {
        // Not a compressed DXn format - attempt to handle simple uncompressed RGBA / RGB (common DDS flags not exhaustively handled)
        // We'll attempt to interpret remaining bytes as linear 32bpp RGBA if enough data
        size_t expected = size_t(width) * size_t(height) * 4;
        if(data.size() - data_offset >= expected) {
            const uint8_t* src = data.data() + data_offset;
            for(size_t i=0;i<size_t(width)*size_t(height);++i){
                Pixel px;
                px.r = src[i*4 + 0];
                px.g = src[i*4 + 1];
                px.b = src[i*4 + 2];
                px.a = src[i*4 + 3];
                out.pixels[i] = px;
            }
            return out;
        } else {
            throw std::runtime_error("Unsupported DDS pixel format or too little data (only DXT1/DXT3/DXT5 and simple 32bpp are supported). Detected fourCC='" + fourcc + "'");
        }
    }
}




void ConverterImage_PTX::ImageToFile(const Image& image_data, const std::string& path_to_output_image){
    throw std::runtime_error("TODO void ConverterImage_PTX::ImageToFile(const Image& image_data, const std::string& path_to_output_image)");
}
