#pragma once

// ESP32-S3 pin map for the I2S mic (INMP441) and amp (MAX98357A).
// Both parts share one I2S bus, so change the wiring here and nowhere else.
// Plain int, not gpio_num_t, so host tests can include this file without ESP-IDF.

namespace anc {

inline constexpr int kPinI2sBclk = 4;   // mic SCK, amp BCLK
inline constexpr int kPinI2sWs   = 5;   // mic WS, amp LRC
inline constexpr int kPinI2sDout = 6;   // amp DIN
inline constexpr int kPinI2sDin  = 7;   // mic SD
inline constexpr int kPinSync1   = 15;  // reserved for multi-node sync (M7)
inline constexpr int kPinSync2   = 16;  // reserved for multi-node sync (M7)

}  // namespace anc
