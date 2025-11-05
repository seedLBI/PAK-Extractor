#include "XorIStream.h"

void XorIStream::seek(size_t pos) {
        file_pos = pos;
        fin.seekg(pos);
    }

    size_t XorIStream::tell() const {
        return file_pos;
    }

    bool XorIStream::good() const {
        return fin.good();
    }

    uint8_t XorIStream::read_u8() {
        char c;
        if (!fin.get(c)) {
            throw std::runtime_error("Failed to read byte");
        }
        uint8_t b = static_cast<uint8_t>(c);
        if (!key.empty()) {
            b ^= key[file_pos % key.size()];
        }
        ++file_pos;
        return b;
    }

    uint16_t XorIStream::read_u16_le() {
        uint16_t v = 0;
        v |= static_cast<uint16_t>(read_u8());
        v |= static_cast<uint16_t>(read_u8()) << 8;
        return v;
    }

    uint32_t XorIStream::read_u32_le() {
        uint32_t v = 0;
        v |= static_cast<uint32_t>(read_u8());
        v |= static_cast<uint32_t>(read_u8()) << 8;
        v |= static_cast<uint32_t>(read_u8()) << 16;
        v |= static_cast<uint32_t>(read_u8()) << 24;
        return v;
    }

    uint64_t XorIStream::read_u64_le() {
        uint64_t v = 0;
        v |= static_cast<uint64_t>(read_u8());
        v |= static_cast<uint64_t>(read_u8()) << 8;
        v |= static_cast<uint64_t>(read_u8()) << 16;
        v |= static_cast<uint64_t>(read_u8()) << 24;
        v |= static_cast<uint64_t>(read_u8()) << 32;
        v |= static_cast<uint64_t>(read_u8()) << 40;
        v |= static_cast<uint64_t>(read_u8()) << 48;
        v |= static_cast<uint64_t>(read_u8()) << 56;
        return v;
    }

    std::string XorIStream::read_string(size_t len) {
        std::string s(len, '\0');
        for (size_t i = 0; i < len; ++i) {
            s[i] = static_cast<char>(read_u8());
        }
        return s;
    }

    void XorIStream::read_bytes(uint8_t* buf, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            buf[i] = read_u8();
        }
    }

    void XorIStream::skip(size_t len) {
        for (size_t i = 0; i < len; ++i) {
            read_u8();
        }
    }
