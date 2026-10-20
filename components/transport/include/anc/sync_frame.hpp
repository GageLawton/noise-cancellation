#pragma once

#include <cstddef>
#include <cstdint>

#include "anc/crc8.hpp"
#include "anc/sync_message.hpp"

namespace anc {

// A SyncMessage as it goes on the wire between nodes (M7): the 20 message
// bytes followed by their CRC-8. The receiver recomputes the CRC, so a
// frame corrupted on the way is thrown away instead of trusted.
inline constexpr std::size_t kSyncFrameBytes = kSyncMessageBytes + 1;

// Writes the message bytes, then their checksum in the last byte.
inline void writeFrame(const SyncMessage& m, uint8_t out[kSyncFrameBytes]) {
    serialize(m, out);
    out[kSyncMessageBytes] = crc8(out, kSyncMessageBytes);
}

// Returns false, leaving m untouched, if in is null, len is not exactly
// kSyncFrameBytes, or the last byte doesn't match the CRC of the rest.
inline bool readFrame(const uint8_t* in, std::size_t len, SyncMessage& m) {
    if (in == nullptr || len != kSyncFrameBytes)
        return false;
    if (in[kSyncMessageBytes] != crc8(in, kSyncMessageBytes))
        return false;
    return deserialize(in, kSyncMessageBytes, m);
}

}  // namespace anc
