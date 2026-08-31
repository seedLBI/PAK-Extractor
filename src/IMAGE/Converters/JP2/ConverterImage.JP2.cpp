#include "ConverterImage.JP2.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <memory>
#include <cstring>

ConverterImage_JP2::ConverterImage_JP2() : ConverterImage("jp2"){
    //std::cout << "ConverterImage_JP2::ConverterImage_JP2()\n";
}

ConverterImage_JP2::~ConverterImage_JP2(){
    //std::cout << "ConverterImage_JP2::~ConverterImage_JP2()\n";
}

uint8_t ConverterImage_JP2::clamp8(const int& v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (uint8_t)v;
}

Image ConverterImage_JP2::FileToImage(const std::vector<uint8_t>& data){
    ConvertedResult.pixels.clear();

    if(image_opj != nullptr){
        opj_image_destroy(image_opj);
        image_opj = nullptr;
    }

    if(!Decode(data)){
        std::cout << "[FileToImage(const std::vector<uint8_t>& data][JP2] can't decode\n";
    }
    if(!Translate()){
        std::cout << "[FileToImage(const std::vector<uint8_t>& data][JP2] can't translate\n";
    }

    if(image_opj != nullptr){
        opj_image_destroy(image_opj);
        image_opj = nullptr;
    }


    return ConvertedResult;
}

Image ConverterImage_JP2::FileToImage(const std::string& path_to_image){
    ConvertedResult.pixels.clear();
    if(image_opj != nullptr){
        opj_image_destroy(image_opj);
        image_opj = nullptr;
    }

    if(!Decode(path_to_image)){
        std::cout << "[FileToImage(const std::string& path_to_image)][JP2] can't decode\n";
    }
    if(!Translate()){
        std::cout << "[FileToImage(const std::string& path_to_image)][JP2] can't translate\n";
    }

    if(image_opj != nullptr){
        opj_image_destroy(image_opj);
        image_opj = nullptr;
    }

    return ConvertedResult;
}

void ConverterImage_JP2::ImageToFile(const Image& image_data, const std::string& path_to_output_image){
    // Проверка на валидность переданных данных
    if (image_data.width <= 0 || image_data.height <= 0 || image_data.pixels.empty()) {
        std::cerr << "[ImageToFile][JP2] Invalid image data\n";
        return;
    }

    // Определяем количество каналов (от 1 до 4, по умолчанию берем 4, если данные RGBA)
    int numcomps = (image_data.channels > 0 && image_data.channels <= 4) ? image_data.channels : 4;

    // Настройка параметров компонентов изображения
    std::vector<opj_image_cmptparm_t> cmptparm(numcomps);
    std::memset(cmptparm.data(), 0, numcomps * sizeof(opj_image_cmptparm_t));

    for (int i = 0; i < numcomps; ++i) {
        cmptparm[i].prec = 8;          // 8 бит на канал
        cmptparm[i].bpp = 8;           // глубина цвета компонента
        cmptparm[i].sgnd = 0;          // Беззнаковые данные (unsigned)
        cmptparm[i].dx = 1;            // Шаг сетки X (субсэмплинг)
        cmptparm[i].dy = 1;            // Шаг сетки Y
        cmptparm[i].w = image_data.width;
        cmptparm[i].h = image_data.height;
    }

    // Определяем цветовое пространство (SRGB для 3/4 каналов, GRAY для 1/2)
    COLOR_SPACE color_space = (numcomps >= 3) ? OPJ_CLRSPC_SRGB : OPJ_CLRSPC_GRAY;

    // Создаем внутреннее представление изображения OpenJPEG
    opj_image_t* image = opj_image_create(numcomps, cmptparm.data(), color_space);
    if (!image) {
        std::cerr << "[ImageToFile][JP2] Failed to create opj_image\n";
        return;
    }

    image->x0 = 0;
    image->y0 = 0;
    image->x1 = image_data.width;
    image->y1 = image_data.height;

    // Переносим пиксели из структуры Image в компоненты OpenJPEG
    int num_pixels = image_data.width * image_data.height;
    for (int i = 0; i < num_pixels; ++i) {
        const auto& pixel = image_data.pixels[i];

        // Маппинг данных по каналам
        if (numcomps > 0) image->comps[0].data[i] = pixel.r;
        if (numcomps > 1) image->comps[1].data[i] = pixel.g;
        if (numcomps > 2) image->comps[2].data[i] = pixel.b;
        if (numcomps > 3) image->comps[3].data[i] = pixel.a;
    }

    // Настраиваем параметры сжатия энкодера
    opj_cparameters_t parameters;
    opj_set_default_encoder_parameters(&parameters);

    // Дефолтные настройки для JPEG 2000
    parameters.tcp_numlayers = 6;
    //parameters.tcp_numresolutions = 6;
    parameters.tcp_mct = (numcomps >= 3) ? 1 : 0; // Использовать Multiple Component Transform (MCT) для RGB

    // Создаем кодек для сжатия (OPJ_CODEC_JP2 - формат файла .jp2)
    opj_codec_t* codec = opj_create_compress(OPJ_CODEC_JP2);
    if (!codec) {
        std::cerr << "[ImageToFile][JP2] Failed to create OpenJPEG codec\n";
        opj_image_destroy(image);
        return;
    }

    // Привязываем параметры к кодеку
    if (!opj_setup_encoder(codec, &parameters, image)) {
        std::cerr << "[ImageToFile][JP2] Failed to setup OpenJPEG encoder\n";
        opj_destroy_codec(codec);
        opj_image_destroy(image);
        return;
    }

    // Открываем файловый поток (OPJ_FALSE означает, что мы открываем поток для записи, а не для чтения)
    opj_stream_t* stream = opj_stream_create_default_file_stream(path_to_output_image.c_str(), OPJ_FALSE);
    if (!stream) {
        std::cerr << "[ImageToFile][JP2] Failed to open output stream: " << path_to_output_image << "\n";
        opj_destroy_codec(codec);
        opj_image_destroy(image);
        return;
    }

    // Начинаем сжатие
    if (!opj_start_compress(codec, image, stream)) {
        std::cerr << "[ImageToFile][JP2] Failed to start compression\n";
        opj_stream_destroy(stream);
        opj_destroy_codec(codec);
        opj_image_destroy(image);
        return;
    }

    // Выполняем кодирование
    if (!opj_encode(codec, stream)) {
        std::cerr << "[ImageToFile][JP2] Failed to encode image\n";
        opj_stream_destroy(stream);
        opj_destroy_codec(codec);
        opj_image_destroy(image);
        return;
    }

    // Завершаем процесс сжатия
    if (!opj_end_compress(codec, stream)) {
        std::cerr << "[ImageToFile][JP2] Failed to finalize compression\n";
    }
    else {
        std::cout << "[ImageToFile][JP2] Image successfully saved to: " << path_to_output_image << "\n";
    }

    // Освобождаем выделенную память
    opj_stream_destroy(stream);
    opj_destroy_codec(codec);
    opj_image_destroy(image);
}

OPJ_SIZE_T ConverterImage_JP2::opj_memory_read(void* buffer, OPJ_SIZE_T nb_bytes, void* user_data) {
        OpjMemStream* ms = reinterpret_cast<OpjMemStream*>(user_data);
        if (!ms || ms->offset >= ms->size) {
            return (OPJ_SIZE_T)-1; // EOF / error
        }
        OPJ_SIZE_T to_read = nb_bytes;
        if (to_read > (ms->size - ms->offset)) {
            to_read = ms->size - ms->offset;
        }
        std::memcpy(buffer, ms->data + ms->offset, to_read);
        ms->offset += to_read;
        return to_read;
}

OPJ_OFF_T ConverterImage_JP2::opj_memory_skip(OPJ_OFF_T nb_bytes, void* user_data) {
        OpjMemStream* ms = reinterpret_cast<OpjMemStream*>(user_data);
        if (!ms || nb_bytes < 0) return -1;
        OPJ_SIZE_T rem = ms->size - ms->offset;
        OPJ_SIZE_T to_skip = static_cast<OPJ_SIZE_T>(nb_bytes);
        if (to_skip > rem) to_skip = rem;
        ms->offset += to_skip;
        return static_cast<OPJ_OFF_T>(to_skip);
}

OPJ_BOOL ConverterImage_JP2::opj_memory_seek(OPJ_OFF_T nb_bytes, void* user_data) {
        OpjMemStream* ms = reinterpret_cast<OpjMemStream*>(user_data);
        if (!ms || nb_bytes < 0) return OPJ_FALSE;
        if ((OPJ_SIZE_T)nb_bytes > ms->size) return OPJ_FALSE;
        ms->offset = static_cast<OPJ_SIZE_T>(nb_bytes);
        return OPJ_TRUE;
}

void ConverterImage_JP2::opj_memory_free_user_data(void*) {

}

bool ConverterImage_JP2::decode_from_stream(opj_stream_t* stream, opj_image_t*& image_opj, const opj_dparameters_t& parameters) {
        if (!stream) {
            std::cerr << "decode_from_stream: null stream\n";
            return false;
        }

        opj_codec_t* codec = opj_create_decompress(OPJ_CODEC_JP2);
        if (!codec) {
            std::cerr << "Failed to create OpenJPEG codec.\n";
            return false;
        }

        if (!opj_setup_decoder(codec, &const_cast<opj_dparameters_t&>(parameters))) {
            std::cerr << "opj_setup_decoder failed\n";
            opj_destroy_codec(codec);
            return false;
        }

        image_opj = nullptr;
        if (!opj_read_header(stream, codec, &image_opj)) {
            std::cerr << "opj_read_header failed\n";
            opj_destroy_codec(codec);
            return false;
        }

        if (!opj_decode(codec, stream, image_opj)) {
            std::cerr << "opj_decode failed\n";
            opj_image_destroy(image_opj);
            image_opj = nullptr;
            opj_destroy_codec(codec);
            return false;
        }

        if (!opj_end_decompress(codec, stream)) {
            std::cerr << "opj_end_decompress failed\n";
        }

        opj_destroy_codec(codec);
        return true;
}


bool ConverterImage_JP2::Decode(const std::vector<uint8_t>& data){
    if (data.empty()) {
        std::cerr << "Decode: input buffer is empty\n";
        return false;
    }

    opj_dparameters_t parameters;
    opj_set_default_decoder_parameters(&parameters);

    OpjMemStream memStream;
    memStream.data = reinterpret_cast<const OPJ_BYTE*>(data.data());
    memStream.size = static_cast<OPJ_SIZE_T>(data.size());
    memStream.offset = 0;

    opj_stream_t* stream = opj_stream_default_create(OPJ_TRUE);
    if (!stream) {
        std::cerr << "Failed to create OpenJPEG default stream\n";
        return false;
    }

    opj_stream_set_read_function(stream, opj_memory_read);
    opj_stream_set_seek_function(stream, opj_memory_seek);
    opj_stream_set_skip_function(stream, opj_memory_skip);
    opj_stream_set_user_data(stream, &memStream, opj_memory_free_user_data);
    opj_stream_set_user_data_length(stream, memStream.size);

    image_opj = nullptr;
    bool ok = decode_from_stream(stream, image_opj, parameters);

    opj_stream_destroy(stream);
    if (!ok) {
        image_opj = nullptr;
        return false;
    }

    return true;
}

bool ConverterImage_JP2::Decode(const std::string& filename){
    opj_dparameters_t parameters;
    opj_set_default_decoder_parameters(&parameters);

    // create a stream
    opj_stream_t* stream = opj_stream_create_default_file_stream(filename.c_str(), true);
    if (!stream) {
        std::cerr << "Failed to open file stream: " << filename << std::endl;
        return false;
    }

    // create a codec for JP2 (supports both JP2 and J2K)
    opj_codec_t* codec = opj_create_decompress(OPJ_CODEC_JP2);
    if (!codec) {
        std::cerr << "Failed to create OpenJPEG codec." << std::endl;
        opj_stream_destroy(stream);
        return false;
    }

    if (!opj_setup_decoder(codec, &parameters)) {
        std::cerr << "opj_setup_decoder failed" << std::endl;
        opj_destroy_codec(codec);
        opj_stream_destroy(stream);
        return false;
    }

    image_opj = nullptr;
    if (!opj_read_header(stream, codec, &image_opj)) {
        std::cerr << "opj_read_header failed" << std::endl;
        opj_destroy_codec(codec);
        opj_stream_destroy(stream);
        return false;
    }

    if (!opj_decode(codec, stream, image_opj)) {
        std::cerr << "opj_decode failed" << std::endl;
        opj_image_destroy(image_opj);
        image_opj = nullptr;
        opj_destroy_codec(codec);
        opj_stream_destroy(stream);
        return false;
    }

    if (!opj_end_decompress(codec, stream)) {
        std::cerr << "opj_end_decompress failed" << std::endl;
    }

    opj_destroy_codec(codec);
    opj_stream_destroy(stream);

    return true;
}

bool ConverterImage_JP2::Translate(){
    if (!image_opj) return false;
    if (image_opj->numcomps < 1) return false;

    int width = image_opj->x1 - image_opj->x0;
    int height = image_opj->y1 - image_opj->y0;
    if (width <= 0 || height <= 0) return false;

    int comps = image_opj->numcomps;
    int channels = comps; // we'll adjust below for YCbCr etc

    // We'll support 1,3,4 component images. If other component counts exist,
    // this example will try to write first 3 components as RGB.
    if (comps < 1) return false;

    // Helper to normalize component sample to 0..255
    auto sample_to_u8 = [](int sample, int prec, bool sgnd) -> uint8_t {
        if (prec <= 8) {
            // simple path: range fits 0..255 or signed -128..127
            if (!sgnd) {
                // unsigned 0..(2^prec - 1)
                int maxv = (1 << prec) - 1;
                int v = (sample * 255 + (maxv/2)) / maxv; // integer rounding
                return clamp8(v);
            } else {
                int minv = -(1 << (prec - 1));
                int maxv = (1 << (prec - 1)) - 1;
                int v = (((sample - minv) * 255) + ((maxv - minv)/2)) / (maxv - minv);
                return clamp8(v);
            }
        } else {
            // general path for prec > 8 (e.g. 10/12/16)
            double minv = sgnd ? -(double)(1 << (prec - 1)) : 0.0;
            double maxv = sgnd ? (double)((1 << (prec - 1)) - 1) : (double)((1 << prec) - 1);
            double normalized = (sample - minv) / (maxv - minv);
            int iv = (int)std::round(normalized * 255.0);
            return clamp8(iv);
        }
    };

    // Read component pointers
    std::vector<opj_image_comp_t*> compsPtr;
    compsPtr.reserve(comps);
    for (int c = 0; c < comps; ++c) compsPtr.push_back(&image_opj->comps[c]);

    // Check that component sizes match expected width/height (if not, we still attempt safe indexing)
    for (int c = 0; c < comps; ++c) {
        if (compsPtr[c]->w != width || compsPtr[c]->h != height) {
            std::cerr << "Warning: component " << c << " has different dimensions (" << compsPtr[c]->w << "x" << compsPtr[c]->h << ") than image (" << width << "x" << height << ")." << std::endl;
            // We'll still use width/height for loop bounds; but indexing will be (y * comp->w + x)
        }
    }




    // If color space is EYCC (JP2 YCbCr), we'll convert to RGB
    bool is_eycc = (image_opj->color_space == OPJ_CLRSPC_EYCC);
    if (is_eycc && comps < 3) is_eycc = false; // safety

    // Prepare output channel count
    if (is_eycc) channels = 3;
    else channels = std::min(comps, 4);



    ConvertedResult.width = width;
    ConvertedResult.height = height;
    ConvertedResult.channels = 4;
    ConvertedResult.pixels.resize( width * height);


    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {

            // For each pixel gather component samples
            // Note: components might have slightly different w/h. Use their own widths when indexing.
            int idx_out = (y * width + x);
            auto &out = ConvertedResult.pixels[idx_out];
            out.a = 255;

            if (is_eycc) {
                // Y, Cb, Cr -> R/G/B
                int w0 = compsPtr[0]->w;
                int w1 = compsPtr[1]->w;
                int w2 = compsPtr[2]->w;
                int idx0 = y * w0 + x;
                int idx1 = y * w1 + x;
                int idx2 = y * w2 + x;
                int Ys = compsPtr[0]->data[idx0];
                int Cbs = compsPtr[1]->data[idx1];
                int Crs = compsPtr[2]->data[idx2];

                uint8_t Y = sample_to_u8(Ys, compsPtr[0]->prec, compsPtr[0]->sgnd);
                uint8_t Cb = sample_to_u8(Cbs, compsPtr[1]->prec, compsPtr[1]->sgnd);
                uint8_t Cr = sample_to_u8(Crs, compsPtr[2]->prec, compsPtr[2]->sgnd);

                // YCbCr (studio range full) -> RGB; using common ITU-R BT.601-ish formula
                int C = (int)Y;
                int D = (int)Cb - 128;
                int E = (int)Cr - 128;
                int R = (int)std::round(C + 1.402 * E);
                int G = (int)std::round(C - 0.344136 * D - 0.714136 * E);
                int B = (int)std::round(C + 1.772 * D);

                out.r = clamp8(R);
                out.g = clamp8(G);
                out.b = clamp8(B);
                out.a = 255;

            } else {
                // Not EYCC: copy first up to 4 components (convert each to 8-bit)
                for (int c = 0; c < channels; ++c) {
                    opj_image_comp_t* comp = compsPtr[c];
                    int cw = comp->w;
                    int idxc = y * cw + x;
                    int sample = comp->data[idxc];
                    uint8_t v = sample_to_u8(sample, comp->prec, comp->sgnd);
                    switch(c){
                        case 0: out.r = v; break;
                        case 1: out.g = v; break;
                        case 2: out.b = v; break;
                        case 3: out.a = 255; break;
                        default: std::cout << "WTF??\n";
                    }
                }
                // If original has 3 comps but channels==4 we won't zero alpha; but channels is min(comps,4)
            }
        }
    }
    return true;
}

