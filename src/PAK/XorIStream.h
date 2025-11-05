#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include <filesystem>
#include <stdexcept>

class XorIStream {
private:
    std::ifstream& fin;
    std::vector<uint8_t> key;
    size_t file_pos = 0;

public:
    XorIStream(std::ifstream& f, const std::vector<uint8_t>& k) : fin(f), key(k) {}

    void seek(size_t pos);
    size_t tell() const;
    bool good() const;

    uint8_t read_u8();
    uint16_t read_u16_le();
    uint32_t read_u32_le();
    uint64_t read_u64_le();
    std::string read_string(size_t len);
    void read_bytes(uint8_t* buf, size_t len);

    void skip(size_t len);
};
