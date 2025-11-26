#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <boost/asio.hpp>

// Helpers to encode/decode 4-byte big-endian length prefix
inline std::vector<uint8_t> make_frame(const std::string& payload) {
    uint32_t len = static_cast<uint32_t>(payload.size());
    std::vector<uint8_t> out(4 + payload.size());
    out[0] = static_cast<uint8_t>((len >> 24) & 0xFF);
    out[1] = static_cast<uint8_t>((len >> 16) & 0xFF);
    out[2] = static_cast<uint8_t>((len >> 8) & 0xFF);
    out[3] = static_cast<uint8_t>((len) & 0xFF);
    std::copy(payload.begin(), payload.end(), out.begin() + 4);
    return out;
}

inline uint32_t parse_length(const std::vector<uint8_t>& buf) {
    if (buf.size() < 4) return 0;
    return (static_cast<uint32_t>(buf[0]) << 24) |
           (static_cast<uint32_t>(buf[1]) << 16) |
           (static_cast<uint32_t>(buf[2]) << 8) |
           (static_cast<uint32_t>(buf[3]));
}
