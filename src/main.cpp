#include "PAK/ExtractorPAK.h"
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
void enable_ansi() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}
#endif

int main(int argc, char* argv[]) {
    ExtractorPAK_POPCAP extractorPAK;

    std::vector<std::filesystem::path> paths;

    std::string path_nice = "";
    std::string path_raw = "";

    for(int i = 1; i < argc; i++){
        std::string str_arg = argv[i];

        if(str_arg != "-raw" && str_arg != "-nice"){
            paths.push_back(str_arg);
        }
        else if(str_arg == "-raw"){
            if(i + 1 > (argc - 1)){
                std::cout << "Where path for folder -raw?\n";
                exit(-1);
            }

            path_raw = argv[i+1];
            i++;
            continue;
        }
        else if(str_arg == "-nice"){
            if(i + 1 > (argc - 1)){
                std::cout << "Where path for folder -nice?\n";
                exit(-1);
            }

            path_nice = argv[i+1];
            i++;
            continue;
        }
    }

    if(path_raw.empty() && path_nice.empty()){
        std::cout << "\"file.pak\" or \"path\\to\\folder\\with\\paks\" and -raw \"output_folder\" and -nice \"output_folder\" \n";
        exit(-1);
    }

    std::cout << "#STARTED CONVERTING\n";

    for(const auto& entry : paths){

        if(entry.has_extension() && entry.extension() == ".pak"){
            std::printf("[%s]\n", entry.generic_u8string().c_str());
            extractorPAK.Init(entry.generic_u8string());

            if(!path_nice.empty())
                extractorPAK.ExctractNice(path_nice);
            if(!path_raw.empty())
                extractorPAK.ExctractRAW(path_raw);
        }
        else{
            for(const auto& file : std::filesystem::recursive_directory_iterator(entry)){
                if(std::filesystem::is_regular_file(file)){
                    std::printf("[%s]\n", file.path().generic_u8string().c_str());
                    extractorPAK.Init(file.path().generic_u8string());

                    if(!path_nice.empty())
                        extractorPAK.ExctractNice(path_nice);
                    if(!path_raw.empty())
                        extractorPAK.ExctractRAW(path_raw);
                }
            }
        }
    }

    return 0;
}



