#include "PackerPAK.h"
#include <filesystem>

PackerPAK_POPCAP::PackerPAK_POPCAP(const std::string& original_pak, bool use_comp) 
    : original_pak_path(original_pak), use_compression(use_comp) {}

std::vector<uint8_t> PackerPAK_POPCAP::ReadFileBytes(const std::string& path) {
    std::ifstream fin(path, std::ios::binary | std::ios::ate);
    if (!fin) return {};
    size_t size = fin.tellg();
    fin.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    fin.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

std::vector<uint8_t> PackerPAK_POPCAP::CompressZlib(const std::vector<uint8_t>& src) {
    uLongf dest_len = compressBound(src.size());
    std::vector<uint8_t> dest(dest_len);
    if (compress(dest.data(), &dest_len, src.data(), src.size()) == Z_OK) {
        dest.resize(dest_len);
        return dest;
    }
    return {};
}

std::vector<uint8_t> PackerPAK_POPCAP::ProcessImageChannel(
    const std::string& png_path, const std::string& target_ext, 
    bool is_alpha_mask, const std::string& temp_folder, bool should_downscale)
{
    ConverterImage_PNG png_conv;
    Image source_png = png_conv.FileToImage(png_path);
    
    Image out_img = source_png;
    
    for(size_t i = 0; i < out_img.pixels.size(); i++) {
        if (is_alpha_mask) {
            uint8_t a = source_png.pixels[i].a;
            out_img.pixels[i] = {a, a, a, 255}; 
        } else {
            out_img.pixels[i].a = 255;
        }
    }

    if (should_downscale && !is_alpha_mask && (target_ext == "jp2" || target_ext == "jpg")) {
        out_img = DownscaleImage(out_img, 2);
    }

    std::string temp_file = temp_folder + "/temp_img." + target_ext;
    
    if (target_ext == "jp2") {
        ConverterImage_JP2().ImageToFile(out_img, temp_file);
    } else if (target_ext == "gif") {
        ConverterImage_GIF().ImageToFile(out_img, temp_file);
    } else if (target_ext == "jpg") {
        ConverterImage_JPG().ImageToFile(out_img, temp_file);
    }

    auto bytes = ReadFileBytes(temp_file);
    std::filesystem::remove(temp_file);
    return bytes;
}

bool PackerPAK_POPCAP::Pack(const std::string& input_folder, const std::string& output_pak) {
    ExtractorPAK_POPCAP extractor;
    if (!extractor.Init(original_pak_path)) {
        std::cerr << "Failed to read original PAK for metadata.\n";
        return false;
    }

    struct OutFile {
        std::string name;
        uint32_t size;
        uint32_t xsize;
        uint64_t tstamp;
        std::vector<uint8_t> data;
    };
    std::vector<OutFile> files_to_write;

    std::string temp_dir = input_folder + "/_temp_build";
    std::filesystem::create_directory(temp_dir);

    auto original_entries = extractor.GetEntries();

    std::cout << "Preparing data and splitting textures...\n";
    for (const auto& entry_name : original_entries) {
        std::string ext = "";
        std::string base_name = "";
        auto pos = entry_name.find_last_of('.');
        if(pos != std::string::npos) {
            ext = entry_name.substr(pos + 1);
            base_name = entry_name.substr(0, pos);
        }

        std::vector<uint8_t> raw_data;
        bool handled = false;

        this->use_compression = extractor.IsCompressed();

        if (ext == "jp2" || ext == "gif") {
            std::string expected_png = input_folder + "/";
            bool is_alpha = (ext == "gif" && base_name.back() == '_');
            
            if (is_alpha) expected_png += base_name.substr(0, base_name.size()-1) + ".png";
            else expected_png += base_name + ".png";

            if (std::filesystem::exists(expected_png)) {
                bool should_downscale = false;
                if (!is_alpha) {
                    std::string alpha_name = base_name + "_.gif";
                    auto alpha_raw = extractor.ExtractEntry(alpha_name);
                    auto color_raw = extractor.ExtractEntry(entry_name);

                    if (!alpha_raw.empty() && !color_raw.empty()) {
                        Image orig_alpha = ConverterImage_GIF().FileToImage(alpha_raw);
                        Image orig_color = ConverterImage_JP2().FileToImage(color_raw);
                        if (orig_alpha.width != orig_color.width) {
                            should_downscale = true;
                        }
                    }
                }
                raw_data = ProcessImageChannel(expected_png, ext, is_alpha, temp_dir, should_downscale);
                handled = true;
            }
        } 
        else if (ext == "jpg") {
            bool is_alpha = (!base_name.empty() && base_name.back() == '_');
            std::string expected_png = input_folder + "/";
            if (is_alpha) expected_png += base_name.substr(0, base_name.size()-1) + ".png";
            else expected_png += base_name + ".png";

            if (std::filesystem::exists(expected_png)) {
                bool should_downscale = false;
                if (!is_alpha) {
                    std::string alpha_name = base_name + "_.jpg";
                    auto alpha_raw = extractor.ExtractEntry(alpha_name);
                    auto color_raw = extractor.ExtractEntry(entry_name);

                    if (!alpha_raw.empty() && !color_raw.empty()) {
                        Image orig_alpha = ConverterImage_JPG().FileToImage(alpha_raw);
                        Image orig_color = ConverterImage_JPG().FileToImage(color_raw);
                        if (orig_alpha.width != orig_color.width) {
                            should_downscale = true;
                        }
                    }
                }
                raw_data = ProcessImageChannel(expected_png, ext, is_alpha, temp_dir, should_downscale);
                handled = true;
            }
        }

        if (!handled) {
            std::string path = input_folder + "/" + entry_name;
            if (std::filesystem::exists(path)) {
                raw_data = ReadFileBytes(path);
            } else {
                raw_data = extractor.ExtractEntry(entry_name);
            }
        }

        OutFile out_f;
        out_f.name = entry_name;
        out_f.size = raw_data.size();
        out_f.tstamp = 0;

        bool should_compress = use_compression;
        if (ext == "jp2" || ext == "jpg" || ext == "gif" || ext == "png" || ext == "ogg" || ext == "ptx") {
            should_compress = false;
        }

        if (should_compress) {
            out_f.data = CompressZlib(raw_data);
            out_f.size = out_f.data.size();
            out_f.xsize = raw_data.size();
        }
        else {
            out_f.data = raw_data;
            out_f.size = raw_data.size();
            out_f.xsize = 0;
        }

        files_to_write.push_back(out_f);
    }

    std::filesystem::remove(temp_dir);


    std::ofstream fout(output_pak, std::ios::binary);

    std::vector<uint8_t> out_key = {0xf7}; 
    XorOStream xout(fout, out_key);

    std::cout << "Writing pak header...\n";
    xout.write_u32_le(SIG);
    xout.write_u32_le(0);

    for (size_t i = 0; i < files_to_write.size(); ++i) {
        xout.write_u8(0x00);
        // FIX: Restore PopCap Windows-style path separators
        std::string pak_name = files_to_write[i].name;
        std::replace(pak_name.begin(), pak_name.end(), '/', '\\');

        xout.write_u8(static_cast<uint8_t>(pak_name.size()));
        xout.write_string(pak_name); // Write the corrected string

        xout.write_u32_le(files_to_write[i].size);
        if (use_compression) {
            xout.write_u32_le(files_to_write[i].xsize);
        }
        xout.write_u64_le(files_to_write[i].tstamp);
    }
    xout.write_u8(0x80);

    for (const auto& f : files_to_write) {
        if (use_compression) {
            xout.write_u16_le(0);
        }
        xout.write_bytes(f.data.data(), f.data.size());
    }

    std::cout << "Done.\n";
    return true;
}