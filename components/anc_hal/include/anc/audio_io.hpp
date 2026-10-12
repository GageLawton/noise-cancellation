#pragma once

#include <cstddef>

namespace anc {

class IAudioIo {
public:
    virtual ~IAudioIo() = default;

    // Reads up to n float samples in [-1.0, 1.0] into buf.
    // Returns the number of samples actually read.
    virtual std::size_t readBlock(float* buf, std::size_t n) = 0;

    // Writes up to n float samples in [-1.0, 1.0] from buf.
    // Returns the number of samples actually written.
    virtual std::size_t writeBlock(const float* buf, std::size_t n) = 0;
};

}  // namespace anc
