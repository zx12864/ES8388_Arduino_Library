#include <Wire.h>
#include "ES8388.h"

// 如果與預設不同，請定義 I2C 接腳
// #define SDA_PIN 21
// #define SCL_PIN 22

ES8388 es;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("ES8388 Basic Example");

  // 初始化 I2C
  // Wire.begin(SDA_PIN, SCL_PIN);
  Wire.begin(); 

  // 初始化編解碼器
  Serial.print("Initializing ES8388...");
  if (!es.begin(&Wire)) {
    Serial.println("Failed!");
    while (1);
  }
  Serial.println("OK");

  // 列印暫存器狀態用於除錯
  // es.dumpRegisters();

  // 設定為 16 位元，44.1kHz（從屬模式）
  // 注意：實際 I2S 資料必須由微控制器提供（例如 ESP32 I2S）
  // 此函式庫僅處理編解碼器的 I2C 設定。
  es.config(16, 44100);
  
  // 選擇輸出
  es.setOutput(ES8388_OUTPUT_LOUT1 | ES8388_OUTPUT_ROUT1);
  
  // 設定音量 (0-100)
  // 0 = -30dB（最小），100 = 0dB（最大）
  es.setVolume(80);
  Serial.println("Volume set to 80");

  // 選擇輸入
  es.setInput(ES8388_INPUT_MIC1);
  
  // 設定麥克風增益 (0-24dB)
  es.setMicGain(18);
  Serial.println("Mic Gain set to 18dB");
}

void loop() {
  // 每 5 秒切換靜音以示範控制
  delay(5000);
  es.mute(true);
  Serial.println("Muted");
  
  delay(5000);
  es.mute(false);
  Serial.println("Unmuted");
}
