#pragma once

#include <cstddef>
#include <cstdint>

namespace anc {

// Timing message exchanged between nodes (design doc 5.6).
struct SyncMessage {
    uint64_t nodeId;
    uint64_t timestampUs;
    uint32_t sequenceNumber;
};

// 8 + 8 + 4 bytes on the wire, with no padding.
inline constexpr std::size_t kSyncMessageBytes = 20;

namespace detail {

inline void putLe(uint8_t* out, uint64_t value, std::size_t bytes) {
    for (std::size_t i = 0; i < bytes; ++i) {
        out[i] = static_cast<uint8_t>(value >> (8 * i));
    }
}

inline uint64_t getLe(const uint8_t* in, std::size_t bytes) {
    uint64_t value = 0;
    for (std::size_t i = 0; i < bytes; ++i) {
        value |= static_cast<uint64_t>(in[i]) << (8 * i);
    }
    return value;
}

}  // namespace detail

// Writes the message little-endian, field by field, so the bytes don't
// depend on the compiler's struct layout or the CPU's byte order.
inline void serialize(const SyncMessage& m, uint8_t out[kSyncMessageBytes]) {
    detail::putLe(out, m.nodeId, 8);
    detail::putLe(out + 8, m.timestampUs, 8);
    detail::putLe(out + 16, m.sequenceNumber, 4);
}

// Returns false, leaving m untouched, unless len is exactly
// kSyncMessageBytes.
inline bool deserialize(const uint8_t* in, std::size_t len, SyncMessage& m) {
    if (in == nullptr || len != kSyncMessageBytes)
        return false;
    m.nodeId         = detail::getLe(in, 8);
    m.timestampUs    = detail::getLe(in + 8, 8);
    m.sequenceNumber = static_cast<uint32_t>(detail::getLe(in + 16, 4));
    return true;
}

}  // namespace anc
