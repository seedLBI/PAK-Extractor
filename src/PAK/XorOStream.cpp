#include "XorOStream.h"

void XorOStream::write_u8(uint8_t b) {
    if (!key.empty()) { b ^= key[file_pos % key.size()]; }
    fout.put(static_cast<char>(b));
    ++file_pos;
}

void XorOStream::write_u16_le(uint16_t v) {
    write_u8(static_cast<uint8_t>(v & 0xFF));
    write_u8(static_cast<uint8_t>((v >> 8) & 0xFF));
}

void XorOStream::write_u32_le(uint32_t v) {
    write_u8(static_cast<uint8_t>(v & 0xFF));
    write_u8(static_cast<uint8_t>((v >> 8) & 0xFF));
    write_u8(static_cast<uint8_t>((v >> 16) & 0xFF));
    write_u8(static_cast<uint8_t>((v >> 24) & 0xFF));
}

void XorOStream::write_u64_le(uint64_t v) {
    for(int i = 0; i < 8; ++i) {
        write_u8(static_cast<uint8_t>((v >> (i * 8)) & 0xFF));
    }
}

void XorOStream::write_string(const std::string& s) {
    for (char c : s) { write_u8(static_cast<uint8_t>(c)); }
}

void XorOStream::write_bytes(const uint8_t* buf, size_t len) {
    for (size_t i = 0; i < len; ++i) { write_u8(buf[i]); }
}