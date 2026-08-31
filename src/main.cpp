#define _CRT_SECURE_NO_WARNINGS
#include "PAK/ExtractorPAK.h"
#include "PAK/PackerPAK.h"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <chrono>
#include <thread>

#ifdef _WIN32
#include <windows.h>
enum class ConsoleColor : WORD {
    Default = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
    Red = FOREGROUND_RED | FOREGROUND_INTENSITY,
    Yellow = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
    Green = FOREGROUND_GREEN | FOREGROUND_INTENSITY,
    Cyan = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
    Purple = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
};

void SetColor(ConsoleColor color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), static_cast<WORD>(color));
}

void PrintUsageWin32() {
    SetColor(ConsoleColor::Yellow);
    std::cout << "Использование (Extract):\n";
    SetColor(ConsoleColor::Default);
    std::cout << "\tPAKExtractor.exe -extract";
    SetColor(ConsoleColor::Cyan);
    std::cout << " Папка или файл с .pak";
    SetColor(ConsoleColor::Red);
    std::cout << " Тип вывода(raw, nice)";
    SetColor(ConsoleColor::Green);
    std::cout << " Папка для вывода\n";

    SetColor(ConsoleColor::Yellow);
    std::cout << "Пример 1.:\n";
    SetColor(ConsoleColor::Default);
    std::cout << "\tPAKExtractor.exe -extract";
    SetColor(ConsoleColor::Cyan);
    std::cout << " main.pak";
    SetColor(ConsoleColor::Red);
    std::cout << " -raw";
    SetColor(ConsoleColor::Green);
    std::cout << " zalupa\n";

    SetColor(ConsoleColor::Yellow);
    std::cout << "Пример 2.:\n";
    SetColor(ConsoleColor::Default);
    std::cout << "\tPAKExtractor.exe -extract";
    SetColor(ConsoleColor::Cyan);
    std::cout << " \"folder_with_paks\"";
    SetColor(ConsoleColor::Red);
    std::cout << " -raw";
    SetColor(ConsoleColor::Green);
    std::cout << " zalupa";

    std::cout << std::endl;
    SetColor(ConsoleColor::Yellow);
    std::cout << "Пояснение:\n";
    SetColor(ConsoleColor::Default);
    std::cout << "\t-raw  - вывод сырых исходников\n";
    std::cout << "\t-nice - объединение пары текстур в один .png с альфа-каналом\n";

    std::cout << std::endl << std::endl;

    SetColor(ConsoleColor::Yellow);
    std::cout << "Использование (Pack):\n";
    SetColor(ConsoleColor::Default);
    std::cout << "\tPAKExtractor.exe -pack";
    SetColor(ConsoleColor::Cyan);
    std::cout << " \"оригинальный.pak\"";
    SetColor(ConsoleColor::Green);
    std::cout << " \"изменённые исходники\"";
    SetColor(ConsoleColor::Purple);
    std::cout << " \"результат.pak\"\n";

    SetColor(ConsoleColor::Yellow);
    std::cout << "Пример:\n";
    SetColor(ConsoleColor::Default);
    std::cout << "\tPAKExtractor.exe -pack";
    SetColor(ConsoleColor::Cyan);
    std::cout << " \"main.pak\"";
    SetColor(ConsoleColor::Green);
    std::cout << " \"zalupa\"";
    SetColor(ConsoleColor::Purple);
    std::cout << " \"result.pak\"\n\n";

    SetColor(ConsoleColor::Default);
    system("pause");
}
#endif

void ProcessPakFile(const std::filesystem::path& filepath, ExtractorPAK_POPCAP& extractor, const std::string& path_nice, const std::string& path_raw) {
    std::cout << "[" << filepath.string() << "]\n";

    extractor.Init(filepath.generic_u8string());

    if (!path_nice.empty()) {
        extractor.ExctractNice(path_nice);
    }
    if (!path_raw.empty()) {
        extractor.ExctractRAW(path_raw);
    }
}

int main(int argc, char* argv[]) {
    std::setlocale(LC_ALL, "Ru");

    std::vector<std::filesystem::path> input_paths;
    std::string path_nice;
    std::string path_raw;
    bool is_extract = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-pack") {
            if (i + 3 >= argc) {
                std::cerr << "Not enough arguments for -pack! Required: <original.pak> <input_folder> <output.pak>\n";
                return 1;
            }
            std::string original_pak = argv[++i];
            std::string input_folder = argv[++i];
            std::string output_pak = argv[++i];

            std::cout << "#STARTED PACKING\n";
            std::cout << "Original: " << original_pak << "\n";
            std::cout << "Folder:   " << input_folder << "\n";
            std::cout << "Output:   " << output_pak << "\n";

            PackerPAK_POPCAP packer(original_pak, true);
            if (packer.Pack(input_folder, output_pak)) {
                std::cout << "Packing successful!\n";
            }
            else {
                std::cerr << "Packing failed!\n";
                return 1;
            }
            return 0;
        }
        else if (arg == "-extract") {
            is_extract = true;
        }
        else if (arg == "-raw") {
            if (i + 1 >= argc) {
                std::cerr << "Missing path for -raw folder!\n";
                return 1;
            }
            path_raw = argv[++i];
        }
        else if (arg == "-nice") {
            if (i + 1 >= argc) {
                std::cerr << "Missing path for -nice folder!\n";
                return 1;
            }
            path_nice = argv[++i];
        }
        else {
            input_paths.push_back(arg);
        }
    }

    if (!is_extract || (path_raw.empty() && path_nice.empty()) || input_paths.empty()) {
#ifdef _WIN32
        PrintUsageWin32();
#else
        std::cout << "Usage: -extract \"file.pak\" -raw \"folder\" OR -extract \"file.pak\" -nice \"folder\"\n";
        std::cout << "Or: -pack \"original.pak\" \"mod_folder\" \"output.pak\"\n";
#endif
        return 1;
    }

    std::cout << "#STARTED EXTRACTING\n";
    ExtractorPAK_POPCAP extractorPAK;

    for (const auto& entry : input_paths) {
        if (!std::filesystem::exists(entry)) {
            std::cerr << "Path does not exist: " << entry.string() << "\n";
            continue;
        }

        if (std::filesystem::is_regular_file(entry) && entry.extension() == ".pak") {
            ProcessPakFile(entry, extractorPAK, path_nice, path_raw);
        }
        else if (std::filesystem::is_directory(entry)) {
            for (const auto& file : std::filesystem::recursive_directory_iterator(entry)) {
                if (std::filesystem::is_regular_file(file) && file.path().extension() == ".pak") {
                    ProcessPakFile(file.path(), extractorPAK, path_nice, path_raw);
                }
            }
        }
    }

    return 0;
}