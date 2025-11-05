#include "ExtractorPAK.h"

#include "IMAGE/Converters/PNG/ConverterImage.PNG.h"
#include "IMAGE/Converters/JP2/ConverterImage.JP2.h"
#include "IMAGE/Converters/GIF/ConverterImage.GIF.h"
#include "IMAGE/Converters/PTX/ConverterImage.PTX.h"
#include "IMAGE/ScalingImage.h"

#include <unordered_map>

ExtractorPAK_POPCAP::ExtractorPAK_POPCAP(const bool& use_compression, const std::string& user_password){
    ExtractorPAK_POPCAP();

    passwords.push_back(std::string("1celowniczy23osral4kibel"));
    passwords.push_back(std::string("www#quarterdigi@com"));
    passwords.push_back(std::string("bigfish"));
    passwords.push_back("");

    this->use_compression = use_compression;

    if (!user_password.empty()) {
        passwords.insert(passwords.begin(), user_password);
    }
}

ExtractorPAK_POPCAP::ExtractorPAK_POPCAP(){
    passwords.push_back(std::string("1celowniczy23osral4kibel"));
    passwords.push_back(std::string("www#quarterdigi@com"));
    passwords.push_back(std::string("bigfish"));
    passwords.push_back("");
    this->use_compression = false;
}

uint64_t ExtractorPAK_POPCAP::GetSizeFile(const std::string path_input_file){
    std::filesystem::path p(path_input_file);
    return std::filesystem::file_size(p);
}

uint64_t ExtractorPAK_POPCAP::GetSizeEntries(){
    uint64_t result = 0;
    for(const auto& entry: entries){
        result += entry.size;
        result += entry.name.size();
    }
    return result;
}

bool ExtractorPAK_POPCAP::TryInit(const std::string path_input_file, bool use_compression){
    this->use_compression = use_compression;

    if(fin){
        fin.close();
    }

    xor_key.clear();
    entries.clear();
    data_offset = 0;
    sign = 0;
    ver = 0;
    xstream = nullptr;


    if(!OpenFile(path_input_file)){
        std::cout << "ERROR: can't open file\n";
        return false;
    }
    if(!GetXorKey()){
        std::cout << "ERROR: can't get xorKey'\n";
        return false;
    }
    if(!ReadEntries()){
        std::cout << "ERROR: Signature mismatch after key selection'\n";
        return false;
    }

    uint64_t size_file = GetSizeFile(path_input_file);
    uint64_t check_size = size_file - (entries.back().block_offset + entries.back().size);


    if(check_size < 50){
        return true;
    }
    else
        return false;
}

bool ExtractorPAK_POPCAP::Init(const std::string path_input_file){

    if(TryInit(path_input_file,false)){
        return true;
    }
    if(TryInit(path_input_file,true)){
        return true;
    }

    return false;
}

bool ExtractorPAK_POPCAP::ExctractRAW(const std::string path_output_folder) {
    CreateOutputFolder(path_output_folder);
    ExtractEntries(path_output_folder);
    return true;
}

bool ExtractorPAK_POPCAP::OpenFile(const std::string& path_input_file){
    fin = std::ifstream(path_input_file, std::ios::binary);
    if (!fin)
        return false;
    return true;
}

bool ExtractorPAK_POPCAP::GetXorKey(){

    // Try passwords
    for (const auto& pw : passwords) {
        std::vector<uint8_t> temp_key;
        for (char c : pw) {
            temp_key.push_back(static_cast<uint8_t>(c));
        }
        XorIStream xstream(fin, temp_key);
        xstream.seek(0);
        uint32_t sign = xstream.read_u32_le();
        if (sign == SIG) {
            xor_key = temp_key;
            return true;
        }
    }

    // Try fixed 0xf7
    std::vector<uint8_t> temp_key = {0xf7};
    XorIStream xstream(fin, temp_key);
    xstream.seek(0);
    uint32_t sign = xstream.read_u32_le();
    if (sign == SIG) {
        xor_key = temp_key;
        return true;
    }

    // Scan single-byte keys from 0xff to 0x01
    for (int i = 0xff; i > 0; --i) {
        std::vector<uint8_t> temp_key = {static_cast<uint8_t>(i)};
        XorIStream xstream(fin, temp_key);
        xstream.seek(0);
        uint32_t sign = xstream.read_u32_le();
        if (sign == SIG) {
            xor_key = temp_key;
            return true;
        }
    }

    return false;
}

bool ExtractorPAK_POPCAP::ReadEntries(){
    xstream = new XorIStream(fin, xor_key);
    xstream->seek(0);

    sign = xstream->read_u32_le();
    if (sign != SIG) { return false; }
    ver = xstream->read_u32_le();

    while (true) {
        uint8_t flags = xstream->read_u8();
        if (flags & 0x80) {
            break;
        }
        uint8_t fnamesz = xstream->read_u8();
        std::string fname = xstream->read_string(fnamesz);
        std::replace(fname.begin(), fname.end(), '\\', '/');
        uint32_t size = xstream->read_u32_le();
        uint32_t xsize = 0;
        if (use_compression) {
            xsize = xstream->read_u32_le();
        }
        uint64_t tstamp = xstream->read_u64_le();
        entries.push_back({fname, size, xsize, tstamp, 0, 0});
    }

    data_offset = xstream->tell();

    uint64_t current_block_offset = data_offset;
    for (auto& entry : entries) {
        entry.block_offset = current_block_offset;
        xstream->seek(current_block_offset);
        if (use_compression) {
            uint16_t num = xstream->read_u16_le();
            xstream->seek(xstream->tell() + num);
            entry.data_offset = xstream->tell();
        } else {
            entry.data_offset = current_block_offset;
        }
        current_block_offset = entry.data_offset + entry.size;
    }


    return true;
}

std::vector<uint8_t> ExtractorPAK_POPCAP::ExtractEntry(const std::string& name){
    FileEntry entry = FindEntry(name);
    if(entry.size == 0){
        return {};
    }

    return ExtractEntry(entry);
}

ExtractorPAK_POPCAP::FileEntry ExtractorPAK_POPCAP::FindEntry(const std::string& name){
    for(size_t i = 0; i < entries.size(); i++){
        if(entries[i].name == name){
            return entries[i];
        }
    }
    return {};
}

std::vector<std::string> ExtractorPAK_POPCAP::GetEntries(){
    std::vector<std::string> result(entries.size());

    for(size_t i = 0; i < entries.size(); i++){
        result[i] = entries[i].name;
    }

    return result;
}

void ExtractorPAK_POPCAP::PrintEntries(){
    std::cout << entries.size() << " of entries\n";
    for(size_t i = 0; i < entries.size(); i++) {
        printf("[%s]-[%i]\n", entries[i].name.c_str(), entries[i].size);
    }
}

std::vector<std::string> ExtractorPAK_POPCAP::GetExistExtensions(){

    std::unordered_map<std::string,int> extensions;

    for(size_t i = 0; i < entries.size(); i++){

        auto pos = entries[i].name.find_last_of('.');
        if(pos != std::string::npos){
            std::string ext = entries[i].name.substr(pos + 1);
            /*
            std::string file_name = entries[i].name.substr(0,pos);

            if(ext == "jp2"){
                int index_find = -1;
                std::string to_find = file_name + "_.gif";
                for(size_t j = 0; j < entries.size(); j++){
                    if(entries[j].name == to_find){
                        index_find = j;
                        break;
                    }
                }
                if(index_find != -1){
                    std::cout << "YEEES";
                }
            }
            printf("[%s]-[%s]-[%s]\n",entries[i].name.c_str(),file_name.c_str(),ext.c_str());
            */

            extensions[ext]++;
        }

    }

    std::vector<std::string> result;
    for(const std::pair<const std::string, int>& ext : extensions){
        result.push_back(ext.first);
    }
    return result;
}

void ExtractorPAK_POPCAP::CreateOutputFolder(const std::string path_output_folder){
    std::filesystem::create_directory(path_output_folder);
}

std::vector<uint8_t> ExtractorPAK_POPCAP::ExtractEntry(const FileEntry& entry){
    std::vector<uint8_t> result;

    bool is_compressed = (entry.xsize != 0);
    uint32_t zsize = entry.size;
    uint32_t usize = is_compressed ? entry.xsize : entry.size;
    xstream->seek(entry.data_offset);

    if (is_compressed) {
        // Read compressed data into buffer
        std::vector<uint8_t> compressed_data(zsize);
        xstream->read_bytes(compressed_data.data(), zsize);

        // Decompress with zlib
        std::vector<uint8_t> decompressed_data(usize);
        uLongf dest_len = usize;
        int ret = uncompress(decompressed_data.data(), &dest_len, compressed_data.data(), zsize);
        if (ret != Z_OK) {
            std::cerr << "Decompression failed for " << entry.name << " (error: " << ret << ")" << std::endl;
            return {};
        }
        if (dest_len != usize) {
            std::cerr << "Decompressed size mismatch for " << entry.name << std::endl;
            return {};
        }
        result = decompressed_data;
    } else {
        result.resize(usize);
        xstream->read_bytes(result.data(), usize);
    }

    return result;
}

void ExtractorPAK_POPCAP::ExtractEntries(const std::string path_output_folder){
    for (const auto& entry : entries) {
        auto data = ExtractEntry(entry);
        if (data.empty()) {
            std::cerr << "Failed to extract " << entry.name << std::endl;
            continue;
        }

        std::string outpath = path_output_folder + "/" + entry.name;
        std::filesystem::path p(outpath);
        std::filesystem::create_directories(p.parent_path());

        std::ofstream fout(outpath, std::ios::binary);
        if (!fout) {
            std::cerr << "Failed to create output file: " << outpath << std::endl;
            continue;
        }

        fout.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
}

bool ExtractorPAK_POPCAP::ExctractNice(const std::string& path_output_folder){
    CreateOutputFolder(path_output_folder);

    std::vector<FileEntry> entr = entries;

    ConverterImage_GIF gif;
    ConverterImage_JP2 jp2;
    ConverterImage_PNG png;
    ConverterImage_PTX ptx;


    int begin_size = entr.size();
    int size_converted_bytes = 0;


    std::cout << "Progress: 0%" << std::endl;
    std::cout << "File: " << std::endl;
    std::cout << "Size converted: 0 bytes" << std::endl;


    while(entr.size() > 0){
        const FileEntry& entry = entr.back();
        size_converted_bytes += entry.size;
        std::string entry_name = entry.name;
        if (entry_name.length() > 40) {
            entry_name = entry_name.substr(0, 37) + "...";
        }


        std::string outpath = path_output_folder + "/" + entry.name;
        std::string extension = "";
        std::string fileName = "";

        std::filesystem::path p(outpath);
        std::filesystem::create_directories(p.parent_path());

        auto pos = entry.name.find_last_of('.');
        if(pos != std::string::npos){
            extension = entry.name.substr(pos + 1);
            fileName = entry.name.substr(0, pos);
        }

        std::vector<uint8_t> data = ExtractEntry(entry);

        if( extension == "ptx"){
            png.ImageToFile(
                    ptx.FileToImage(data),
                    path_output_folder + "/" + fileName + ".png"
            );
        }
        else if( extension == "jp2" || extension == "gif" ){

            std::vector<uint8_t> data_second;
            Image alpha_from_gif;
            Image color_from_jp2;

            int index_find = -1;

            if(extension == "jp2"){
                std::string to_find = fileName + "_.gif";
                for(size_t j = 0; j < entr.size(); j++){
                    if(entr[j].name == to_find){
                        index_find = j;
                        break;
                    }
                }
            }
            else{
                std::string to_find = fileName.substr(0,fileName.size()-1) + ".jp2";
                for(size_t j = 0; j < entr.size(); j++){
                    if(entr[j].name == to_find){
                        index_find = j;
                        break;
                    }
                }
            }

            if(extension == "jp2")
                color_from_jp2 = jp2.FileToImage(data);
            else
                alpha_from_gif = gif.FileToImage(data);



            if(index_find != -1){
                size_converted_bytes += entr[index_find].size;
                data_second = ExtractEntry(entr[index_find]);
                if(extension == "jp2")
                    alpha_from_gif = gif.FileToImage(data_second);
                else
                    color_from_jp2 = jp2.FileToImage(data_second);

                if(alpha_from_gif.width != color_from_jp2.width){
                    color_from_jp2 = BicubicScaleImage(color_from_jp2,2);
                }

                if(alpha_from_gif.width != color_from_jp2.width){
                    std::cout << "NO WAY!!\n";
                    exit(228);
                }

                for(size_t i = 0; i < color_from_jp2.pixels.size(); i++){
                    color_from_jp2.pixels[i].a = alpha_from_gif.pixels[i].r;
                }

                png.ImageToFile(
                    color_from_jp2,
                    path_output_folder + "/" + fileName + ".png"
                );

                entr.erase(entr.begin() + index_find);
            }
            else{
                if(extension == "jp2") {
                    png.ImageToFile(
                        color_from_jp2,
                        path_output_folder + "/" + fileName + ".png"
                    );
                }
                else {
                    for(size_t i = 0; i < alpha_from_gif.pixels.size(); i++){
                        alpha_from_gif.pixels[i].a = alpha_from_gif.pixels[i].r;
                    }
                    png.ImageToFile(
                        alpha_from_gif,
                        path_output_folder + "/" + fileName.substr(0,fileName.size()-1) + ".png"
                    );
                }
            }


        }
        else{

            std::ofstream fout(outpath, std::ios::binary);
            if (!fout) {
                std::cerr << "Failed to create output file: " << outpath << std::endl;
                continue;
            }
            for(size_t i = 0; i< data.size(); i++) {
                fout.put(static_cast<char>(data[i]));
            }
        }


        entr.pop_back();

        double progress = (1.0 - ((double)entr.size() / (double)begin_size)) * 100.0;

        std::cout << "\033[3A";
        std::cout << "Progress: " << std::fixed << std::setprecision(0) << progress << "%" << "\033[K" << std::endl;
        std::cout << "File: " << entry_name << "\033[K" << std::endl;
        std::cout << "Size converted: " << size_converted_bytes << " bytes" << "\033[K" << std::endl << std::flush;

    }

    std::cout << "\033[3A";
    std::cout << "  \033[1;31m Size converted: " << size_converted_bytes << " bytes" << "\033[K \033[1;37m" << std::endl;


    return true;
}
