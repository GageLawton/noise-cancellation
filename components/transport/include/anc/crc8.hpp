#pragma once

#include <cstddef>
#include <cstdint>

namespace anc {

// One-byte checksum for messages between nodes (M7). A corrupted
// SyncMessage gives a different CRC, so the receiver can throw it away.
//
// CRC-8/SMBUS: polynomial 0x07, initial value 0x00, no reflection, no
// final XOR. The standard check value is crc8("123456789") == 0xF4.
inline uint8_t crc8(const uint8_t* data, std::size_t len) {
    uint8_t crc = 0x00;
    for (std::size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            // Shift left; if the top bit fell off, XOR in the polynomial.
            crc = (crc & 0x80u) ? static_cast<uint8_t>((crc << 1) ^ 0x07u)
                                : static_cast<uint8_t>(crc << 1);
        }
    }
    return crc;
}

}  // namespace anc
