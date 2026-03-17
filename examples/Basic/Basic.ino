#include <Wire.h>
#include <ES8388.h>

ES8388 codec;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("ES8388 Basic Example");

  // Initialize I2C (optional, can be done inside begin if default pins are used)
  // Wire.begin(SDA_PIN, SCL_PIN); 

  if (!codec.begin()) {
    Serial.println("Failed to initialize ES8388!");
    while (1);
  }

  Serial.println("ES8388 Initialized");

  // Set Volume (0-100)
  codec.setVolume(75);
  Serial.println("Volume set to 75");

  // Configure for 16-bit, 44.1kHz (Slave mode)
  // Note: The actual I2S data must be provided by the microcontroller (e.g. ESP32 I2S)
  codec.config(16, 44100);
  
  // Select Output
  codec.setOutput(ES8388_OUTPUT_LOUT1 | ES8388_OUTPUT_ROUT1);
}

void loop() {
  // Toggle mute every 5 seconds
  delay(5000);
  codec.mute(true);
  Serial.println("Muted");
  
  delay(5000);
  codec.mute(false);
  Serial.println("Unmuted");
}
