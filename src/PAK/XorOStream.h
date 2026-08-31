#ifndef XOROSTREAM_H
#define XOROSTREAM_H

#include <fstream>
#include <vector>
#include <cstdint>
#include <string>

class XorOStream {
private:
    std::ofstream& fout;
    std::vector<uint8_t> key;
    size_t file_pos = 0;

public:
    XorOStream(std::ofstream& f, const std::vector<uint8_t>& k) : fout(f), key(k) {}

    void write_u8(uint8_t b);

    void write_u16_le(uint16_t v);

    void write_u32_le(uint32_t v);

    void write_u64_le(uint64_t v);

    void write_string(const std::string& s);

    void write_bytes(const uint8_t* buf, size_t len);
};

#endif