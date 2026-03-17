#ifndef ES8388_H
#define ES8388_H

#include <Arduino.h>
#include <Wire.h>

// ES8388 I2C 位址
#define ES8388_ADDR 0x10

// 暫存器映射
#define ES8388_CONTROL1         0x00
#define ES8388_CONTROL2         0x01
#define ES8388_CHIPPOWER        0x02
#define ES8388_ADCPOWER         0x03
#define ES8388_DACPOWER         0x04
#define ES8388_CHIPLOPOW1       0x05
#define ES8388_CHIPLOPOW2       0x06
#define ES8388_ANAVOLMANAG      0x07
#define ES8388_MASTERMODE       0x08
#define ES8388_ADCCONTROL1      0x09
#define ES8388_ADCCONTROL2      0x0A
#define ES8388_ADCCONTROL3      0x0B
#define ES8388_ADCCONTROL4      0x0C
#define ES8388_ADCCONTROL5      0x0D
#define ES8388_ADCCONTROL6      0x0E
#define ES8388_ADCCONTROL7      0x0F
#define ES8388_ADCCONTROL8      0x10
#define ES8388_ADCCONTROL9      0x11
#define ES8388_ADCCONTROL10     0x12
#define ES8388_ADCCONTROL11     0x13
#define ES8388_ADCCONTROL12     0x14
#define ES8388_ADCCONTROL13     0x15
#define ES8388_ADCCONTROL14     0x16
#define ES8388_DACCONTROL1      0x17
#define ES8388_DACCONTROL2      0x18
#define ES8388_DACCONTROL3      0x19
#define ES8388_DACCONTROL4      0x1A
#define ES8388_DACCONTROL5      0x1B
#define ES8388_DACCONTROL6      0x1C
#define ES8388_DACCONTROL7      0x1D
#define ES8388_DACCONTROL8      0x1E
#define ES8388_DACCONTROL9      0x1F
#define ES8388_DACCONTROL10     0x20
#define ES8388_DACCONTROL11     0x21
#define ES8388_DACCONTROL12     0x22
#define ES8388_DACCONTROL13     0x23
#define ES8388_DACCONTROL14     0x24
#define ES8388_DACCONTROL15     0x25
#define ES8388_DACCONTROL16     0x26
#define ES8388_DACCONTROL17     0x27
#define ES8388_DACCONTROL18     0x28
#define ES8388_DACCONTROL19     0x29
#define ES8388_DACCONTROL20     0x2A
#define ES8388_DACCONTROL21     0x2B
#define ES8388_DACCONTROL22     0x2C
#define ES8388_DACCONTROL23     0x2D
#define ES8388_DACCONTROL24     0x2E
#define ES8388_DACCONTROL25     0x2F
#define ES8388_DACCONTROL26     0x30
#define ES8388_DACCONTROL27     0x31
#define ES8388_DACCONTROL28     0x32
#define ES8388_DACCONTROL29     0x33
#define ES8388_DACCONTROL30     0x34

// I2S 格式列舉
typedef enum {
    ES8388_I2S_NORMAL = 0,
    ES8388_I2S_LEFT,
    ES8388_I2S_RIGHT,
    ES8388_I2S_DSP
} ES8388_I2S_Fmt;

// 輸出通道定義
#define ES8388_OUTPUT_LOUT1     0x01
#define ES8388_OUTPUT_LOUT2     0x02
#define ES8388_OUTPUT_ROUT1     0x04
#define ES8388_OUTPUT_ROUT2     0x08

// 類比輸入路徑
typedef enum {
    ES8388_MIC1 = 0,
    ES8388_MIC2,
    ES8388_LINE1,
    ES8388_LINE2,
    ES8388_DIFF_MIC1, // 差分 MIC1
    ES8388_DIFF_MIC2  // 差分 MIC2
} ES8388_InputPath;

class ES8388 {
public:
    ES8388();
    
    // 初始化編解碼器
    bool begin(TwoWire *wire = &Wire);
    
    // 系統控制
    void reset();
    void dumpRegisters();

    // 電源管理
    void powerDown();
    void powerUp();
    void standby();
    
    // 音量控制
    void setVolume(uint8_t volume); // 0-100
    void mute(bool enable);
    void muteDAC(bool enable);
    void muteADC(bool enable);
    
    // 輸入/輸出路徑
    void setInput(ES8388_InputPath input);
    void setInput(uint8_t input); // 相容舊 API
    void setOutput(uint8_t output); // 使用 ES8388_OUTPUT_xxx 常數
    void setOutputMixer(bool left_mix, bool right_mix);
    void enableAnalogBypass(bool enable); // 啟用類比直通 (Mic -> Output)

    // I2S 設定
    void config(uint8_t bits, uint32_t sampleRate);
    void setI2SFormat(ES8388_I2S_Fmt fmt);
    void setMasterMode(bool enable);

    // ADC 進階控制
    void setMicGain(uint8_t gainDb); // 0-24 dB (每 3dB 一階)
    void setALC(bool enable, uint8_t maxGain, uint8_t minGain, uint8_t target, uint8_t holdTime, uint8_t decayTime, uint8_t attackTime);
    void setNoiseGate(uint8_t threshold); // 0-31
    void setHPF(bool enable, uint8_t cutoff = 0);

    // DAC 進階控制
    void setDeemphasis(bool enable); // 44.1kHz 去加重
    void setVolumeRaw(uint8_t vol_l, uint8_t vol_r);

    // 直接存取暫存器
    void writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);

private:
    TwoWire *_wire;
    uint8_t _address;
    
    // 內部輔助函式
    void setBitsPerSample(uint8_t bits);
    void setSampleRate(uint32_t rate);
};

#endif
