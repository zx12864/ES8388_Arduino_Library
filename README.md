# ES8388 Arduino Library

An Arduino library for controlling the **Everest Semi ES8388** audio codec via I2C. 

This library handles the configuration and control of the ES8388 chip (volume, input/output selection, format, etc.). **Note:** This library only manages the **I2C control interface**. The audio data transmission (I2S) must be handled by your microcontroller's I2S driver (e.g., using the `ESP32-audioI2S` library or native I2S drivers).

## Features

- **Initialization**: Easy setup via I2C.
- **Volume Control**: Set analog output volume (0-100) mapping to -30dB to 0dB.
- **Input Selection**: Switch between MIC1, MIC2.
- **Output Selection**: Enable/Disable LOUT1, ROUT1, LOUT2, ROUT2.
- **Format Configuration**: Support for 16/24/32-bit data and various sample rates.
- **Microphone Gain**: Adjustable microphone gain (0-24dB).
- **Noise Gate**: Configurable noise gate threshold.

## Hardware Connection

The ES8388 communicates configuration via I2C and audio data via I2S.

### I2C Connection (Control)
| ES8388 Pin | Arduino/MCU Pin |
|:----------:|:---------------:|
| VCC        | 3.3V            |
| GND        | GND             |
| SDA        | SDA (GPIO 21 on ESP32) |
| SCL        | SCL (GPIO 22 on ESP32) |

### I2S Connection (Audio Data)
| ES8388 Pin | Arduino/MCU Pin (Example ESP32) |
|:----------:|:-------------------------------:|
| MCLK       | I2S MCLK (GPIO 0)               |
| BCLK       | I2S BCLK (GPIO 27)              |
| LRCK       | I2S LRCK (GPIO 26)              |
| DADC (OUT) | I2S DIN  (GPIO 35)              |
| DACD (IN)  | I2S DOUT (GPIO 25)              |

*> Note: Pin mappings depend on your specific development board and configuration.*

## Usage

### Include the Library
```cpp
#include <Wire.h>
#include "ES8388.h"
```

### Initialization
```cpp
ES8388 es;

void setup() {
  Wire.begin(); // Initialize I2C
  
  // Start the codec
  if (!es.begin(&Wire)) {
    Serial.println("ES8388 not found!");
    while(1);
  }
  
  // Configure codec (16-bit, 44.1kHz)
  es.config(16, 44100);
  
  // Set initial volume (0-100)
  // 0 = -30dB (Min), 100 = 0dB (Max)
  es.setVolume(80);
  
  // Select Output (LOUT1/ROUT1)
  es.setOutput(ES8388_OUTPUT_LOUT1 | ES8388_OUTPUT_ROUT1);
  
  // Select Input (MIC1)
  es.setInput(ES8388_INPUT_MIC1);
  es.setMicGain(18); // Set Mic Gain to 18dB
}
```

## API Reference

### `bool begin(TwoWire *wire = &Wire)`
Initializes the ES8388 codec. Returns `true` if successful, `false` otherwise.

### `void config(uint8_t bits, uint32_t sampleRate)`
Configures the audio format (16/24/32 bits) and sample rate.

### `void setVolume(uint8_t volume)`
Sets the analog output volume.
- `volume`: 0 to 100.
  - 0 maps to -30dB.
  - 100 maps to 0dB.

### `void mute(bool enable)`
Mutes or unmutes the output.
- `enable`: `true` to mute, `false` to unmute.

### `void setInput(uint8_t input)`
Selects the input source.
- `input`: 
    - `ES8388_INPUT_MIC1`
    - `ES8388_INPUT_MIC2`

### `void setOutput(uint8_t output)`
Selects the active outputs. Can be combined using bitwise OR `|`.
- `output`:
    - `ES8388_OUTPUT_LOUT1`
    - `ES8388_OUTPUT_ROUT1`
    - `ES8388_OUTPUT_LOUT2`
    - `ES8388_OUTPUT_ROUT2`

### `void setMicGain(uint8_t gainDb)`
Sets the microphone input gain.
- `gainDb`: 0 to 24 dB (in 3dB steps).

### `void setNoiseGate(uint8_t threshold)`
Sets the noise gate threshold.
- `threshold`: 0 to 31.

### `void setHPF(bool enable)`
Enables or disables the High Pass Filter (HPF) for ADC.
- `enable`: `true` to enable, `false` to disable.
