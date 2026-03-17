#include <Wire.h>
#include "ES8388.h"

// Define I2C Pins if different from default
// #define SDA_PIN 21
// #define SCL_PIN 22

ES8388 es;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("ES8388 Basic Example");

  // Initialize I2C
  // Wire.begin(SDA_PIN, SCL_PIN);
  Wire.begin(); 

  // Initialize the codec
  Serial.print("Initializing ES8388...");
  if (!es.begin(&Wire)) {
    Serial.println("Failed!");
    while (1);
  }
  Serial.println("OK");

  // Configure for 16-bit, 44.1kHz (Slave mode)
  // Note: The actual I2S data must be provided by the microcontroller (e.g. ESP32 I2S)
  // This library only handles the I2C configuration of the codec.
  es.config(16, 44100);
  
  // Select Output
  es.setOutput(ES8388_OUTPUT_LOUT1 | ES8388_OUTPUT_ROUT1);
  
  // Set Volume (0-100)
  // 0 = -30dB (Min), 100 = 0dB (Max)
  es.setVolume(80);
  Serial.println("Volume set to 80");

  // Select Input
  es.setInput(ES8388_INPUT_MIC1);
  
  // Set Microphone Gain (0-24dB)
  es.setMicGain(18);
  Serial.println("Mic Gain set to 18dB");
}

void loop() {
  // Toggle mute every 5 seconds to demonstrate control
  delay(5000);
  es.mute(true);
  Serial.println("Muted");
  
  delay(5000);
  es.mute(false);
  Serial.println("Unmuted");
}
