#include "ExtractorPAK.h"

#include "IMAGE/Converters/PNG/ConverterImage.PNG.h"
#include "IMAGE/Converters/JP2/ConverterImage.JP2.h"
#include "IMAGE/Converters/GIF/ConverterImage.GIF.h"
#include "IMAGE/Converters/PTX/ConverterImage.PTX.h"
#include "IMAGE/ScalingImage.h"

#include <unordered_map>
#include <iomanip>
#include <sstream>


std::string FormatSize(uint64_t bytes) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2);
    if (bytes < 1024) {
        out << bytes << " b";
    }
    else if (bytes < 1024 * 1024) {
        out << (bytes / 1024.0) << " Kb";
    }
    else if (bytes < 1024 * 1024 * 1024) {
        out << (bytes / (1024.0 * 1024.0)) << " Mb";
    }
    else {
        out << (bytes / (1024.0 * 1024.0 * 1024.0)) << " Gb";
    }
    return out.str();
}

ExtractorPAK_POPCAP::ExtractorPAK_POPCAP(const bool& use_compression, const std::string& user_password) {
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

ExtractorPAK_POPCAP::ExtractorPAK_POPCAP() {
    passwords.push_back(std::string("1celowniczy23osral4kibel"));
    passwords.push_back(std::string("www#quarterdigi@com"));
    passwords.push_back(std::string("bigfish"));
    passwords.push_back("");
    this->use_compression = false;
}

uint64_t ExtractorPAK_POPCAP::GetSizeFile(const std::string path_input_file) {
    std::filesystem::path p(path_input_file);
    return std::filesystem::file_size(p);
}

uint64_t ExtractorPAK_POPCAP::GetSizeEntries() {
    uint64_t result = 0;
    for (const auto& entry : entries) {
        result += entry.size;
        result += entry.name.size();
    }
    return result;
}

bool ExtractorPAK_POPCAP::TryInit(const std::string path_input_file, bool use_compression) {
    std::cout << "[TryInit] Попытка чтения (Сжатие: " << (use_compression ? "ВКЛ" : "ВЫКЛ") << ")...\n";

    this->use_compression = use_compression;

    if (fin) {
        fin.close();
    }

    xor_key.clear();
    entries.clear();
    data_offset = 0;
    sign = 0;
    ver = 0;
    xstream = nullptr;


    if (!OpenFile(path_input_file)) {
        std::cout << "ERROR: can't open file\n";
        return false;
    }
    if (!GetXorKey()) {
        std::cout << "ERROR: can't get xorKey'\n";
        return false;
    }
    if (!ReadEntries()) {
        std::cout << "ERROR: Signature mismatch after key selection'\n";
        return false;
    }

    uint64_t size_file = GetSizeFile(path_input_file);
    uint64_t check_size = size_file - (entries.back().block_offset + entries.back().size);


    std::cout << "[TryInit] " << (check_size < 50 ? "УСПЕХ" : "НЕУДАЧА") << std::endl;
    return check_size < 50;
}

bool ExtractorPAK_POPCAP::Init(const std::string path_input_file) {
    std::cout << "\n========================================\n";
    std::cout << "[Init] Полный путь к файлу: " << std::filesystem::absolute(path_input_file).string() << "\n";
    std::cout << "[Init] Размер файла: " << GetSizeFile(path_input_file) << " байт\n";
    std::cout << "========================================\n";

    if (TryInit(path_input_file, false)) {
        return true;
    }
    if (TryInit(path_input_file, true)) {
        return true;
    }

    return false;
}

bool ExtractorPAK_POPCAP::OpenFile(const std::string& path_input_file) {
    std::filesystem::path p(path_input_file);
    this->input_filename = p.filename().string();

    fin = std::ifstream(path_input_file, std::ios::binary);
    if (!fin)
        return false;
    return true;
}

bool ExtractorPAK_POPCAP::GetXorKey() {
    std::cout << "[GetXorKey] Поиск подходящего ключа или пароля...\n";

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
            std::cout << "[GetXorKey] УСПЕХ: Найден пароль -> '" << (pw.empty() ? "<без пароля>" : pw) << "'\n";
            return true;
        }
    }

    // Try fixed 0xf7
    std::vector<uint8_t> temp_key = { 0xf7 };
    XorIStream xstream(fin, temp_key);
    xstream.seek(0);
    uint32_t sign = xstream.read_u32_le();
    if (sign == SIG) {
        xor_key = temp_key;
        std::cout << "[GetXorKey] УСПЕХ: Найден фиксированный ключ -> 0xf7\n";
        return true;
    }

    // Scan single-byte keys from 0xff to 0x01
    for (int i = 0xff; i > 0; --i) {
        std::vector<uint8_t> temp_key = { static_cast<uint8_t>(i) };
        XorIStream xstream(fin, temp_key);
        xstream.seek(0);
        uint32_t sign = xstream.read_u32_le();
        if (sign == SIG) {
            xor_key = temp_key;
            std::cout << "[GetXorKey] УСПЕХ: Найден однобайтовый ключ -> 0x" << std::hex << i << std::dec << "\n";
            return true;
        }
    }
    std::cout << "[GetXorKey] ОШИБКА: Подходящий ключ не найден.\n";

    return false;
}

bool ExtractorPAK_POPCAP::ReadEntries() {
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
        entries.push_back({ fname, size, xsize, tstamp, 0, 0 });
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
        }
        else {
            entry.data_offset = current_block_offset;
        }
        current_block_offset = entry.data_offset + entry.size;
    }


    return true;
}

std::vector<uint8_t> ExtractorPAK_POPCAP::ExtractEntry(const std::string& name) {
    FileEntry entry = FindEntry(name);
    if (entry.size == 0) {
        return {};
    }

    return ExtractEntry(entry);
}

ExtractorPAK_POPCAP::FileEntry ExtractorPAK_POPCAP::FindEntry(const std::string& name) {
    for (size_t i = 0; i < entries.size(); i++) {
        if (entries[i].name == name) {
            return entries[i];
        }
    }
    return {};
}

std::vector<std::string> ExtractorPAK_POPCAP::GetEntries() {
    std::vector<std::string> result(entries.size());

    for (size_t i = 0; i < entries.size(); i++) {
        result[i] = entries[i].name;
    }

    return result;
}

void ExtractorPAK_POPCAP::PrintEntries() {
    std::cout << entries.size() << " of entries\n";
    for (size_t i = 0; i < entries.size(); i++) {
        printf("[%s]-[%i]\n", entries[i].name.c_str(), entries[i].size);
    }
}

std::vector<std::string> ExtractorPAK_POPCAP::GetExistExtensions() {

    std::unordered_map<std::string, int> extensions;

    for (size_t i = 0; i < entries.size(); i++) {

        auto pos = entries[i].name.find_last_of('.');
        if (pos != std::string::npos) {
            std::string ext = entries[i].name.substr(pos + 1);
            extensions[ext]++;
        }

    }

    std::vector<std::string> result;
    for (const std::pair<const std::string, int>& ext : extensions) {
        result.push_back(ext.first);
    }
    return result;
}

void ExtractorPAK_POPCAP::CreateOutputFolder(const std::string path_output_folder) {
    std::filesystem::create_directory(path_output_folder);
}

std::vector<uint8_t> ExtractorPAK_POPCAP::ExtractEntry(const FileEntry& entry) {
    std::vector<uint8_t> result;

    bool is_compressed = (entry.xsize != 0);
    uint32_t zsize = entry.size;
    uint32_t usize = is_compressed ? entry.xsize : entry.size;

    xstream->seek(entry.data_offset);

    if (is_compressed) {
        std::vector<uint8_t> compressed_data(zsize);
        xstream->read_bytes(compressed_data.data(), zsize);

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
    }
    else {
        result.resize(usize);
        xstream->read_bytes(result.data(), usize);
    }

    return result;
}

void ExtractorPAK_POPCAP::ExtractEntries(const std::string path_output_folder) {
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

#ifdef _WIN32
#include <windows.h>
#endif

bool ExtractorPAK_POPCAP::ExctractRAW(const std::string path_output_folder) {
    CreateOutputFolder(path_output_folder);
    ExtractEntries(path_output_folder);
    return true;
}

bool ExtractorPAK_POPCAP::ExctractNice(const std::string& path_output_folder) {
    CreateOutputFolder(path_output_folder);

    std::vector<FileEntry> entr = entries;

    ConverterImage_GIF gif;
    ConverterImage_JP2 jp2;
    ConverterImage_JPG jpg;
    ConverterImage_PNG png;
    ConverterImage_PTX ptx;

    int begin_size = entr.size();
    uint64_t size_converted_bytes = 0;

    std::string target_pak_name = input_filename.empty() ? "main.pak" : input_filename;


    std::cout << "==Процесс распаковки (" << target_pak_name << ")==:\n";
    std::cout << "Выполненно: 00.00%\n";
    std::cout << "Пройденный размер: 0 b\n";
    std::cout << "Размер файла: 0 b\n";
    std::cout << "Путь до файла: \"\"\n" << std::flush;

    while (entr.size() > 0) {
        const FileEntry& entry = entr.back();
        size_converted_bytes += entry.size;

        std::string outpath = path_output_folder + "/" + entry.name;
        std::string extension = "";
        std::string fileName = "";

        std::filesystem::path p(outpath);
        std::filesystem::create_directories(p.parent_path());
        
        auto pos = entry.name.find_last_of('.');
        if (pos != std::string::npos) {
            extension = entry.name.substr(pos + 1);
            fileName = entry.name.substr(0, pos);
        }

        std::vector<uint8_t> data = ExtractEntry(entry);

        if (extension == "ptx") {
            png.ImageToFile(
                ptx.FileToImage(data),
                path_output_folder + "/" + fileName + ".png"
            );
        }
        else if (extension == "jp2" || extension == "gif") {

            std::vector<uint8_t> data_second;
            Image alpha_image;
            Image color_image;

            int index_find = -1;
            bool is_alpha = (!fileName.empty() && fileName.back() == '_');

            std::string expected_partner = "";
            bool partner_is_jp2 = false;

            if (extension == "jp2") {
                is_alpha = false;
                expected_partner = fileName + "_.gif";
            }
            else if (extension == "gif") {
                if (is_alpha) {
                    std::string baseName = fileName.substr(0, fileName.size() - 1);
                    std::string to_find_jp2 = baseName + ".jp2";
                    std::string to_find_gif = baseName + ".gif";

                    for (size_t j = 0; j < entr.size(); j++) {
                        if (entr[j].name == to_find_jp2) {
                            index_find = j;
                            partner_is_jp2 = true;
                            break;
                        }
                        else if (entr[j].name == to_find_gif) {
                            index_find = j;
                            partner_is_jp2 = false;
                            break;
                        }
                    }
                }
                else {
                    expected_partner = fileName + "_.gif";
                }
            }

            if (index_find == -1 && !expected_partner.empty()) {
                for (size_t j = 0; j < entr.size(); j++) {
                    if (entr[j].name == expected_partner) {
                        index_find = j;
                        partner_is_jp2 = false;
                        break;
                    }
                }
            }

            if (is_alpha) {
                alpha_image = gif.FileToImage(data);
            }
            else {
                if (extension == "jp2") color_image = jp2.FileToImage(data);
                else color_image = gif.FileToImage(data);
            }

            if (index_find != -1) {
                size_converted_bytes += entr[index_find].size;
                data_second = ExtractEntry(entr[index_find]);

                if (is_alpha) {
                    if (partner_is_jp2) color_image = jp2.FileToImage(data_second);
                    else color_image = gif.FileToImage(data_second);
                }
                else {
                    alpha_image = gif.FileToImage(data_second);
                }

                if (alpha_image.width != color_image.width) {
                    color_image = BicubicScaleImage(color_image, 2);
                }

                if (alpha_image.width != color_image.width) {
                    std::cout << "NO WAY!!\n";
                    exit(228);
                }

                for (size_t i = 0; i < color_image.pixels.size(); i++) {
                    color_image.pixels[i].a = alpha_image.pixels[i].r;
                }

                std::string outName = is_alpha ? fileName.substr(0, fileName.size() - 1) : fileName;

                png.ImageToFile(
                    color_image,
                    path_output_folder + "/" + outName + ".png"
                );

                entr.erase(entr.begin() + index_find);
            }
            else {
                if (!is_alpha) {
                    png.ImageToFile(
                        color_image,
                        path_output_folder + "/" + fileName + ".png"
                    );
                }
                else {
                    for (size_t i = 0; i < alpha_image.pixels.size(); i++) {
                        alpha_image.pixels[i].a = alpha_image.pixels[i].r;
                    }
                    png.ImageToFile(
                        alpha_image,
                        path_output_folder + "/" + fileName.substr(0, fileName.size() - 1) + ".png"
                    );
                }
            }
        }
        else if (extension == "jpg") {
            std::vector<uint8_t> data_second;
            Image alpha_from_jpg;
            Image color_from_jpg;

            int index_find = -1;

            bool is_alpha = (!fileName.empty() && fileName.back() == '_');

            std::string to_find = is_alpha ? (fileName.substr(0, fileName.size() - 1) + ".jpg")
                : (fileName + "_.jpg");

            for (size_t j = 0; j < entr.size(); j++) {
                if (entr[j].name == to_find) {
                    index_find = j;
                    break;
                }
            }

            if (is_alpha) {
                alpha_from_jpg = jpg.FileToImage(data);
            }
            else {
                color_from_jpg = jpg.FileToImage(data);
            }

            if (index_find != -1) {
                size_converted_bytes += entr[index_find].size;
                data_second = ExtractEntry(entr[index_find]);

                if (is_alpha) {
                    color_from_jpg = jpg.FileToImage(data_second);
                }
                else {
                    alpha_from_jpg = jpg.FileToImage(data_second);
                }

                if (alpha_from_jpg.width != color_from_jpg.width) {
                    color_from_jpg = BicubicScaleImage(color_from_jpg, 2);
                }

                if (alpha_from_jpg.width != color_from_jpg.width) {
                    std::cout << "NO WAY!!\n";
                    exit(228);
                }

                for (size_t i = 0; i < color_from_jpg.pixels.size(); i++) {
                    color_from_jpg.pixels[i].a = alpha_from_jpg.pixels[i].r;
                }

                std::string outName = is_alpha ? fileName.substr(0, fileName.size() - 1) : fileName;

                png.ImageToFile(
                    color_from_jpg,
                    path_output_folder + "/" + outName + ".png"
                );

                entr.erase(entr.begin() + index_find);
            }
            else {
                if (!is_alpha) {
                    png.ImageToFile(
                        color_from_jpg,
                        path_output_folder + "/" + fileName + ".png"
                    );
                }
                else {
                    for (size_t i = 0; i < alpha_from_jpg.pixels.size(); i++) {
                        alpha_from_jpg.pixels[i].a = alpha_from_jpg.pixels[i].r;
                    }
                    png.ImageToFile(
                        alpha_from_jpg,
                        path_output_folder + "/" + fileName.substr(0, fileName.size() - 1) + ".png"
                    );
                }
            }
        }
        else {

            std::ofstream fout(outpath, std::ios::binary);
            if (!fout) {
                std::cerr << "Failed to create output file: " << outpath << std::endl;
                continue;
            }
            for (size_t i = 0; i < data.size(); i++) {
                fout.put(static_cast<char>(data[i]));
            }
        }

        double progress = begin_size > 0 ? (1.0 - ((double)entr.size() / (double)begin_size)) * 100.0 : 100.0;
        char prog_buf[16];
        snprintf(prog_buf, sizeof(prog_buf), "%05.2f%%", progress);

        std::string display_path = entry.name;
        std::replace(display_path.begin(), display_path.end(), '/', '\\');
        std::replace(display_path.begin(), display_path.end(), '\n', ' ');
        std::replace(display_path.begin(), display_path.end(), '\r', ' ');
        std::replace(display_path.begin(), display_path.end(), '\t', ' ');
        std::replace(display_path.begin(), display_path.end(), '\v', ' ');

        if (display_path.length() > 35) {
            display_path = "..." + display_path.substr(display_path.length() - 32);
        }

        entr.pop_back();

#ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hConsole, &csbi);

        COORD cursorPos = csbi.dwCursorPosition;
        cursorPos.Y -= 4;
        SetConsoleCursorPosition(hConsole, cursorPos);

        std::string pad(50, ' ');
        std::cout << "Выполненно: " << prog_buf << pad << "\n";
        std::cout << "Пройденный размер: " << FormatSize(size_converted_bytes) << pad << "\n";
        std::cout << "Размер файла: " << FormatSize(entry.size) << pad << "\n";
        std::cout << "Путь до файла: \"" << display_path << "\"" << pad << "\n" << std::flush;
#else
        std::cout << "\033[4A"; 
        std::cout << "Выполненно: " << prog_buf << "\033[K\n";
        std::cout << "Пройденный размер: " << FormatSize(size_converted_bytes) << "\033[K\n";
        std::cout << "Размер файла: " << FormatSize(entry.size) << "\033[K\n";
        std::cout << "Путь до файла: \"" << display_path << "\"\033[K\n" << std::flush;
#endif

    }

#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    COORD cursorPos = csbi.dwCursorPosition;
    cursorPos.Y -= 4;
    SetConsoleCursorPosition(hConsole, cursorPos);

    std::string pad(50, ' ');
    std::cout << "Выполненно: 100.00%" << pad << "\n";
    std::cout << "Пройденный размер: " << FormatSize(size_converted_bytes) << pad << "\n";
    std::cout << "Количество файлов: " << begin_size << pad << "\n";
    std::cout << pad << "\n" << std::flush;
#else
    std::cout << "\033[4A";
    std::cout << "Выполненно: 100.00%\033[K\n";
    std::cout << "Пройденный размер: " << FormatSize(size_converted_bytes) << "\033[K\n";
    std::cout << "Количество файлов: " << begin_size << "\033[K\n";
    std::cout << "\033[K\n" << std::flush;
#endif

    return true;
}