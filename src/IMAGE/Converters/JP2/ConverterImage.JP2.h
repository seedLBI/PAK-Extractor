#ifndef CONVERTER_IMAGE_JP2_H
#define CONVERTER_IMAGE_JP2_H

#include "IMAGE/ConverterImage.h"
#include <openjpeg.h>

class ConverterImage_JP2 : public ConverterImage{
public:
    ConverterImage_JP2();
    ~ConverterImage_JP2();
    Image FileToImage(const std::vector<uint8_t>& data) override;
    Image FileToImage(const std::string& path_to_image) override;
    void ImageToFile(const Image& image_data, const std::string& path_to_output_image) override;
private:
    Image ConvertedResult;
    opj_image_t* image_opj = nullptr;
    bool Decode(const std::vector<uint8_t>& data);
    bool Decode(const std::string& filename);
    bool Translate();

    static uint8_t clamp8(const int& v);


    struct OpjMemStream {
        const OPJ_BYTE* data;
        OPJ_SIZE_T size;
        OPJ_SIZE_T offset;
    };
    static OPJ_SIZE_T opj_memory_read(void* buffer, OPJ_SIZE_T nb_bytes, void* user_data);
    static OPJ_OFF_T opj_memory_skip(OPJ_OFF_T nb_bytes, void* user_data);
    static OPJ_BOOL opj_memory_seek(OPJ_OFF_T nb_bytes, void* user_data);
    static void opj_memory_free_user_data(void* /*p_user_data*/);
    bool decode_from_stream(opj_stream_t* stream, opj_image_t*& image_opj, const opj_dparameters_t& parameters);
};

#endif
