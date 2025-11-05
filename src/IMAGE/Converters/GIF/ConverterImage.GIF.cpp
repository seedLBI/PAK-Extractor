#include "ConverterImage.GIF.h"
#include <iostream>
#include <cstring>

ConverterImage_GIF::ConverterImage_GIF() : ConverterImage("gif"){

}

ConverterImage_GIF::~ConverterImage_GIF(){

}


void ConverterImage_GIF::build_332_palette(GifColorType palette[256]) {
    for (int r = 0; r < 8; ++r) {
        for (int g = 0; g < 8; ++g) {
            for (int b = 0; b < 4; ++b) {
                int idx = (r << 5) | (g << 2) | b;
                uint8_t R = static_cast<uint8_t>((r * 255) / 7);
                uint8_t G = static_cast<uint8_t>((g * 255) / 7);
                uint8_t B = static_cast<uint8_t>((b * 255) / 3);
                palette[idx].Red = R;
                palette[idx].Green = G;
                palette[idx].Blue = B;
            }
        }
    }
}

unsigned char ConverterImage_GIF::color_to_332_index(uint8_t r, uint8_t g, uint8_t b) {
    int ri = (r * 7) / 255;
    int gi = (g * 7) / 255;
    int bi = (b * 3) / 255;
    return static_cast<unsigned char>((ri << 5) | (gi << 2) | bi);
}

Image ConverterImage_GIF::FileToImage(const std::vector<uint8_t>& data){
    if (data.empty()) {
        std::cerr << "Empty input buffer" << std::endl;
        return {};
    }

    // Small helper to keep read state
    struct MemFile {
        const uint8_t* data;
        size_t size;
        size_t pos;
    };

    // GIF read callback: read up to 'size' bytes into buf and return how many bytes were read.
    auto mem_read = [](GifFileType* gif, GifByteType* buf, int size) -> int {
        if (!gif || !gif->UserData || size <= 0) return 0;
        MemFile* mf = static_cast<MemFile*>(gif->UserData);
        size_t remaining = mf->size - mf->pos;
        size_t toRead = (static_cast<size_t>(size) < remaining) ? static_cast<size_t>(size) : remaining;
        if (toRead > 0) {
            memcpy(buf, mf->data + mf->pos, toRead);
            mf->pos += toRead;
        }
        return static_cast<int>(toRead);
    };

    int error = 0;
    // allocate MemFile on heap so it stays alive while giflib uses it
    MemFile* mf = new MemFile{ data.data(), data.size(), 0 };

    // DGifOpen expects a function pointer of type GifInputFunc (int (*)(GifFileType*, GifByteType*, int))
    // cast lambda to function pointer (works because lambda has C calling convention and no captures)
    GifFileType* gif = DGifOpen(static_cast<void*>(mf), mem_read, &error);

    if (!gif) {
        std::cerr << "DGifOpen (memory) failed: error=" << error << std::endl;
        delete mf;
        return {};
    }

    if (DGifSlurp(gif) == GIF_ERROR) {
        std::cerr << "DGifSlurp failed: " << GifErrorString(gif->Error) << std::endl;
        DGifCloseFile(gif, &error);
        delete mf;
        return {};
    }

    if (gif->ImageCount < 1) {
        std::cerr << "No images in GIF" << std::endl;
        DGifCloseFile(gif, &error);
        delete mf;
        return {};
    }

    // Use first frame same as your file path version
    SavedImage& frame = gif->SavedImages[0];
    int width = gif->SWidth;
    int height = gif->SHeight;

    // color map: prefer local (frame) then global
    ColorMapObject* cmap = frame.ImageDesc.ColorMap ? frame.ImageDesc.ColorMap : gif->SColorMap;
    if (!cmap) {
        std::cerr << "No color map in GIF" << std::endl;
        DGifCloseFile(gif, &error);
        delete mf;
        return {};
    }

    // find transparent index from Graphics Control Extension (GCE)
    int transparentIndex = -1;
    for (int i = 0; i < frame.ExtensionBlockCount; ++i) {
        ExtensionBlock& ext = frame.ExtensionBlocks[i];
        if (ext.Function == GRAPHICS_EXT_FUNC_CODE && ext.ByteCount >= 4) {
            const GifByteType* ed = ext.Bytes;
            // packed field: transparent flag is bit 0
            if (ed[0] & 0x01) {
                transparentIndex = static_cast<int>(ed[3]);
            }
            break; // only one GCE expected per frame
        }
    }

    Image img;
    img.width = width;
    img.height = height;
    img.channels = 4;
    img.pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height));

    const GifByteType* raster = frame.RasterBits;
    const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
    for (size_t i = 0; i < pixelCount; ++i) {
        int idx = raster[i];
        // safety: clamp index to color table size
        if (idx < 0) idx = 0;
        if (idx >= static_cast<int>(cmap->ColorCount)) idx = cmap->ColorCount - 1;

        GifColorType col = cmap->Colors[idx];
        Pixel p;
        p.r = col.Red;
        p.g = col.Green;
        p.b = col.Blue;
        p.a = (transparentIndex >= 0 && idx == transparentIndex) ? 0 : 255;
        img.pixels[i] = p;
    }

    // cleanup
    DGifCloseFile(gif, &error);
    delete mf;
    return img;
}

Image ConverterImage_GIF::FileToImage(const std::string& path_to_image){
    int error = 0;
    GifFileType* gif = DGifOpenFileName(path_to_image.c_str(), &error);
    if (!gif) {
        std::cerr << "DGifOpenFileName failed: error=" << error << std::endl;
        return {};
    }

    if (DGifSlurp(gif) == GIF_ERROR) {
        std::cerr << "DGifSlurp failed: " << GifErrorString(gif->Error) << std::endl;
        DGifCloseFile(gif, &error);
        return {};
    }

    if (gif->ImageCount < 1) {
        std::cerr << "No images in GIF" << std::endl;
        DGifCloseFile(gif, &error);
        return {};
    }

    // Берём первый кадр
    SavedImage& frame = gif->SavedImages[0];
    int width = gif->SWidth;
    int height = gif->SHeight;

    // Определяем цветную карту (локальную, иначе глобальную)
    ColorMapObject* cmap = frame.ImageDesc.ColorMap ? frame.ImageDesc.ColorMap : gif->SColorMap;
    if (!cmap) {
        std::cerr << "No color map in GIF" << std::endl;
        DGifCloseFile(gif, &error);
        return {};
    }

    // Попробуем найти прозрачный индекс из расширений (Graphics Control Extension)
    int transparentIndex = -1;
    // SavedImage.Extensions содержит блоки расширений; по спецификации GCE ext_code == GRAPHICS_EXT_FUNC_CODE
    for (int i = 0; i < frame.ExtensionBlockCount; ++i) {
        ExtensionBlock& ext = frame.ExtensionBlocks[i];
        if (ext.Function == GRAPHICS_EXT_FUNC_CODE && ext.ByteCount >= 4) {
            // Формат GCE: 1 байт packed, 2 байта delay, 1 байт transparent color index
            const GifByteType* data = ext.Bytes;
            // transparent flag установлен в младшем бите packed
            if (data[0] & 0x01) {
                transparentIndex = (int)(data[3]);
            }
            break;
        }
    }

    Image img;
    img.width = width;
    img.height = height;
    img.channels = 4;
    img.pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height));

    // RasterBits содержит индексы в палитре (размер width*height)
    const GifByteType* raster = frame.RasterBits;
    const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
    for (size_t i = 0; i < pixelCount; ++i) {
        int idx = raster[i];
        GifColorType col = cmap->Colors[idx];
        Pixel p;
        p.r = col.Red;
        p.g = col.Green;
        p.b = col.Blue;
        if (transparentIndex >= 0 && idx == transparentIndex) p.a = 0;
        else p.a = 255;
        img.pixels[i] = p;
    }

    DGifCloseFile(gif, &error);
    return img;
}


void ConverterImage_GIF::ImageToFile(const Image& image_data, const std::string& path_to_output_image){
    if (image_data.width <= 0 || image_data.height <= 0) {
        std::cerr << "Invalid image size" << std::endl;
        return;
    }

    int error = 0;
    GifFileType* gif = EGifOpenFileName(path_to_output_image.c_str(), false, &error);
    if (!gif) {
        std::cerr << "EGifOpenFileName failed: error=" << error << std::endl;
        return;
    }

    // Построим 256-палитру (3-3-2)
    GifColorType palette[256];
    build_332_palette(palette);
    ColorMapObject* cmap = GifMakeMapObject(256, palette);
    if (!cmap) {
        std::cerr << "GifMakeMapObject failed" << std::endl;
        EGifCloseFile(gif, &error);
        return;
    }

    // Подготовим индексный буфер (width * height)
    const int w = image_data.width;
    const int h = image_data.height;
    std::vector<GifByteType> indices(static_cast<size_t>(w) * static_cast<size_t>(h));

    bool hasTransparency = false;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int idx = y * w + x;
            const Pixel& p = image_data.pixels[idx];
            if (p.a < 128) {
                // пометим как прозрачный (временно) => индекс 0 будет считаться прозрачным
                indices[idx] = 0;
                hasTransparency = true;
            } else {
                // Нормальное квантование (композит на черный фон при наличии альфы)
                uint8_t R = p.r;
                uint8_t G = p.g;
                uint8_t B = p.b;
                // если частично прозрачный, предварительно композит на черный
                if (p.a < 255) {
                    float alpha = p.a / 255.0f;
                    R = static_cast<uint8_t>(R * alpha);
                    G = static_cast<uint8_t>(G * alpha);
                    B = static_cast<uint8_t>(B * alpha);
                }
                indices[idx] = color_to_332_index(R, G, B);
            }
        }
    }

    // Если есть прозрачность - гарантируем, что palette[0] это (0,0,0) (он и так будет)
    if (hasTransparency) {
        cmap->Colors[0].Red = 0;
        cmap->Colors[0].Green = 0;
        cmap->Colors[0].Blue = 0;
    }

    // Запись заголовка экрана
    if (EGifPutScreenDesc(gif, w, h, 8, 0, cmap) == GIF_ERROR) {
        std::cerr << "EGifPutScreenDesc failed: " << GifErrorString(gif->Error) << std::endl;
        GifFreeMapObject(cmap);
        EGifCloseFile(gif, &error);
        return;
    }

    // Если есть прозрачность, добавим Graphics Control Extension с флагом прозрачности и индексом 0
    if (hasTransparency) {
        // GCE data: packed(1 byte), delay low, delay high, transparentIndex
        GifByteType gce[4];
        // packed: бит0 - TransparentColorFlag (1)
        // оставшиеся биты 0 (no disposal specified)
        gce[0] = 0x01; // transparent flag
        gce[1] = 0; // delay low
        gce[2] = 0; // delay high
        gce[3] = 0; // transparent index = 0
        if (EGifPutExtension(gif, GRAPHICS_EXT_FUNC_CODE, sizeof(gce), gce) == GIF_ERROR) {
            std::cerr << "EGifPutExtension (GCE) failed: " << GifErrorString(gif->Error) << std::endl;
            GifFreeMapObject(cmap);
            EGifCloseFile(gif, &error);
            return;
        }
    }

    // Пишем описание изображения (без локальной колormap, так как используем глобальную)
    if (EGifPutImageDesc(gif, 0, 0, w, h, false, nullptr) == GIF_ERROR) {
        std::cerr << "EGifPutImageDesc failed: " << GifErrorString(gif->Error) << std::endl;
        GifFreeMapObject(cmap);
        EGifCloseFile(gif, &error);
        return;
    }

    // Записываем построчно
    for (int y = 0; y < h; ++y) {
        GifByteType* rowPtr = &indices[static_cast<size_t>(y) * static_cast<size_t>(w)];
        if (EGifPutLine(gif, rowPtr, w) == GIF_ERROR) {
            std::cerr << "EGifPutLine failed on row " << y << ": " << GifErrorString(gif->Error) << std::endl;
            GifFreeMapObject(cmap);
            EGifCloseFile(gif, &error);
            return;
        }
    }

    if (EGifCloseFile(gif, &error) == GIF_ERROR) {
        std::cerr << "EGifCloseFile failed: " << GifErrorString(error) << std::endl;
    }

    // cmap был передан в EGifPutScreenDesc; FreeMapObject всё ещё можно освободить
    GifFreeMapObject(cmap);
}
