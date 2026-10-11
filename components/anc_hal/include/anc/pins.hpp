// Pin definitions for the ANC hardware abstraction layer.
// They're plain "int" on purpose so the host test can include the file without ESP-ID.
#pragma once

namespace anc {

// GPIO pin definitions for the ANC hardware abstraction layer.
inline constexpr int kPinI2sBclk = 4; // mic SDK, amp BCLK
inline constexpr int kPinI2sWs = 5; // mic WS, amp LRC
inline constexpr int kPinI2sDout = 6; // amp DIN
inline constexpr int kPinI2sDin = 7; // mic SD
inline constexpr int kPinSync1 = 15; // reserved for multi-node sync (M7)
inline constexpr int kPinSync2 = 16; // reserved for multi-node sync (M7)

}  // namespace anc
