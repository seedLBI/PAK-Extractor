#ifndef EXTRACTOR_PAK_H
#define EXTRACTOR_PAK_H

#include "IMAGE/Converters/PNG/ConverterImage.PNG.h"
#include "IMAGE/Converters/JP2/ConverterImage.JP2.h"
#include "IMAGE/Converters/GIF/ConverterImage.GIF.h"
#include "IMAGE/Converters/PTX/ConverterImage.PTX.h"
#include "IMAGE/ScalingImage.h"

#include "XorIStream.h"
#include <zlib.h>

class ExtractorPAK_POPCAP {
public:
    ExtractorPAK_POPCAP();
    ExtractorPAK_POPCAP(const bool& use_compression, const std::string& user_password);

    bool Init(const std::string path_input_file);
    bool ExctractRAW(const std::string path_output_folder);
    bool ExctractNice(const std::string& path_output_folder);

    std::vector<std::string> GetEntries();
    std::vector<uint8_t> ExtractEntry(const std::string& name);

private:
    struct FileEntry {
std::string name;
        uint32_t size;
        uint32_t xsize;
        uint64_t tstamp;
        uint64_t block_offset;
        uint64_t data_offset;
    };
    const uint32_t SIG = 0xbac04ac0;

    bool use_compression;
    std::vector<std::string> passwords;
    std::vector<uint8_t> xor_key;

    std::ifstream fin;
    XorIStream* xstream;
    uint32_t sign;
    uint32_t ver;
    std::vector<FileEntry> entries;
    size_t data_offset;

    void CreateOutputFolder(const std::string path_output_folder);
    bool OpenFile(const std::string& path_input_file);
    bool GetXorKey();
    bool ReadEntries();

    bool TryInit(const std::string path_input_file, bool use_compression);

    uint64_t GetSizeFile(const std::string path_input_file);
    uint64_t GetSizeEntries();

    std::vector<std::string> GetExistExtensions();

    void PrintEntries();
    FileEntry FindEntry(const std::string& name);
    std::vector<uint8_t> ExtractEntry(const FileEntry& entry);
    void ExtractEntries(const std::string path_output_folder);
};

#endif







