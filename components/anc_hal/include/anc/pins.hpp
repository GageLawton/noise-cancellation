// Pin definitions for the ANC hardware abstraction layer.
#ifndef ANC_PINS_HPP
#define ANC_PINS_HPP

namespace anc {

// GPIO pin definitions for the ANC hardware abstraction layer.
// GPIO 4          I2S bit clock (BCLK)
constexpr int BCLK_PIN = 4;

// GPIO 5          I2S word select (WS)
constexpr int WS_PIN = 5;

// GPIO 6          I2S data out, to amp
constexpr int DIN_PIN = 6;

// GPIO 7          I2S data in, from mic
constexpr int SD_PIN = 7;

// GPIO 15, 16     reserved for multi-node sync (M7)
constexpr int M7_PIN_1 = 15;
constexpr int M7_PIN_2 = 16;

}  // namespace anc

#endif  // ANC_PINS_HPP