#pragma once

#include <cstddef>
#include <cstdint>

namespace anc {

// Estimates how far another node's clock is from ours, and how fast it
// drifts, from pairs of (our time, their time) taken from SyncMessages.
//
// Fits remote = a + b * local by least squares over the last kPairs
// pairs. Times are taken relative to the oldest pair kept before fitting,
// so the sums stay small enough for double even with 64-bit microsecond
// clocks.
class ClockOffset {
public:
    static constexpr std::size_t kPairs   = 16;
    static constexpr std::size_t kMinFits = 4;  // pairs needed for ready()

    // Records one pair, overwriting the oldest once the ring is full.
    void add(uint64_t localUs, uint64_t remoteUs) {
        local_[next_]  = localUs;
        remote_[next_] = remoteUs;
        next_          = (next_ + 1) % kPairs;
        if (count_ < kPairs)
            ++count_;
        fit();
    }

    bool ready() const { return count_ >= kMinFits; }

    // Remote minus local at the newest pair, from the fitted line.
    double offsetUs() const { return offsetUs_; }

    // How much faster the remote clock runs, in parts per million.
    double driftPpm() const { return (slope_ - 1.0) * 1e6; }

private:
    // Index of the i-th pair kept, oldest first.
    std::size_t at(std::size_t i) const {
        return (next_ + kPairs - count_ + i) % kPairs;
    }

    void fit() {
        if (!ready())
            return;
        const uint64_t local0  = local_[at(0)];
        const uint64_t remote0 = remote_[at(0)];

        double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
        for (std::size_t i = 0; i < count_; ++i) {
            const double x =
                static_cast<double>(static_cast<int64_t>(local_[at(i)] - local0));
            const double y =
                static_cast<double>(static_cast<int64_t>(remote_[at(i)] - remote0));
            sx += x;
            sy += y;
            sxx += x * x;
            sxy += x * y;
        }
        const double n     = static_cast<double>(count_);
        const double denom = n * sxx - sx * sx;
        if (denom == 0.0)
            return;  // all local times equal: no line to fit
        slope_                 = (n * sxy - sx * sy) / denom;
        const double intercept = (sy - slope_ * sx) / n;

        // Fitted remote minus local at the newest pair, put back on the
        // original clocks.
        const double xNew =
            static_cast<double>(static_cast<int64_t>(local_[at(count_ - 1)] - local0));
        const double start =
            static_cast<double>(static_cast<int64_t>(remote0 - local0));
        offsetUs_ = start + intercept + (slope_ - 1.0) * xNew;
    }

    uint64_t local_[kPairs]{};
    uint64_t remote_[kPairs]{};
    std::size_t next_  = 0;  // slot the next pair goes into
    std::size_t count_ = 0;  // pairs kept, up to kPairs
    double slope_      = 1.0;
    double offsetUs_   = 0.0;
};

}  // namespace anc
