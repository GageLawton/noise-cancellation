// Pin definitions for the ANC hardware abstraction layer.
#ifndef ANC_PINS_HPP
#define ANC_PINS_HPP

// Example pin definitions (replace with actual hardware-specific pins)
#define ANC_MIC_PIN 0
#define ANC_SPEAKER_PIN 1

/**
 * ESP32-S3 pin mapping for the ANC hardware abstraction layer.
 * 
 * ESP32-S3 pin    Signal          INMP441 mic    MAX98357A amp
 * --------------  --------------  --------------   --------------
 * GPIO 4          I2S bit clock (BCLK)   SCK           BCLK
 * GPIO 5          I2S word select (WS)   WS            LRC
 * GPIO 6          I2S data out, to amp               DIN
 * GPIO 7          I2S data in, from mic   SD            -
 * GPIO 15, 16     reserved for multi-node sync (M7)   -           -
 */ 

namespace anc {

    constexpr int MIC_PIN = ANC_MIC_PIN;
    constexpr int SPEAKER_PIN = ANC_SPEAKER_PIN;    
    constexpr int BCLK_PIN = 4;
    constexpr int WS_PIN = 5;
    constexpr int DIN_PIN = 6;
    constexpr int SD_PIN = 7;
    constexpr int M7_PIN_1 = 15;
    constexpr int M7_PIN_2 = 16;


}  // namespace anc


#endif  // ANC_PINS_HPP