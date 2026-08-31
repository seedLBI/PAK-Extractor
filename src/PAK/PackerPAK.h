#ifndef PACKER_PAK_H
#define PACKER_PAK_H

#include "ExtractorPAK.h"
#include "XorOStream.h"
#include <zlib.h>

class PackerPAK_POPCAP {
public:
    PackerPAK_POPCAP(const std::string& original_pak, bool use_compression = false);

    bool Pack(const std::string& input_folder, const std::string& output_pak);

private:
    std::string original_pak_path;
    bool use_compression = false;
    const uint32_t SIG = 0xbac04ac0;

    std::vector<uint8_t> ReadFileBytes(const std::string& path);
    std::vector<uint8_t> CompressZlib(const std::vector<uint8_t>& src);

    std::vector<uint8_t> ProcessImageChannel(
        const std::string& png_path, 
        const std::string& target_ext, 
        bool is_alpha_mask,
        const std::string& temp_folder,
        bool should_downscale = false
    );
};

#endif