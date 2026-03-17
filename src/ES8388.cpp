#include "ES8388.h"

ES8388::ES8388() {
    _address = ES8388_ADDR;
}

bool ES8388::begin(TwoWire *wire) {
    _wire = wire;
    
    // 檢查裝置是否連接
    _wire->beginTransmission(_address);
    if (_wire->endTransmission() != 0) {
        return false;
    }

    // 軟體重置
    reset();
    
    // 電源管理
    powerUp();
    
    // 從屬模式（預設）
    setMasterMode(false);

    // DAC 控制
    writeRegister(ES8388_DACCONTROL2, 0x02); // DACL/R 混合
    writeRegister(ES8388_DACCONTROL16, 0x00); // 0dB 增益
    writeRegister(ES8388_DACCONTROL17, 0x90); // 啟用左 DAC 到左混音器
    writeRegister(ES8388_DACCONTROL20, 0x90); // 啟用右 DAC 到右混音器
    writeRegister(ES8388_DACCONTROL21, 0x80); // DAC DRC 啟用
    writeRegister(ES8388_DACCONTROL23, 0x00); // 0dB
    
    // 預設類比輸出音量為最大 (0dB)
    writeRegister(ES8388_DACCONTROL24, 0x1E); // LOUT1 音量 0dB
    writeRegister(ES8388_DACCONTROL25, 0x1E); // ROUT1 音量 0dB
    writeRegister(ES8388_DACCONTROL26, 0x1E); // LOUT2 音量 0dB
    writeRegister(ES8388_DACCONTROL27, 0x1E); // ROUT2 音量 0dB

    // ADC 控制
    writeRegister(ES8388_ADCCONTROL1, 0x88); // 麥克風增益 +24dB
    writeRegister(ES8388_ADCCONTROL2, 0xF0); 
    writeRegister(ES8388_ADCCONTROL3, 0x00);
    writeRegister(ES8388_ADCCONTROL5, 0x02); // 0dB

    // 預設 I2S 設定 (16bit, 44100Hz)
    config(16, 44100);

    setVolume(60); // 預設音量
    
    return true;
}

void ES8388::reset() {
    writeRegister(ES8388_DACCONTROL3, 0x04); // 靜音 DAC
    writeRegister(ES8388_CONTROL2, 0x7F);    // 重置所有
    writeRegister(ES8388_CHIPPOWER, 0xF0);   // 關閉電源
    delay(10);
    writeRegister(ES8388_CONTROL2, 0x00);    // 釋放重置
}

void ES8388::dumpRegisters() {
    Serial.println("ES8388 Register Dump:");
    for (int i = 0; i <= 0x34; i++) {
        uint8_t val = readRegister(i);
        Serial.print("Reg 0x");
        if (i < 0x10) Serial.print("0");
        Serial.print(i, HEX);
        Serial.print(": 0x");
        if (val < 0x10) Serial.print("0");
        Serial.println(val, HEX);
    }
}

void ES8388::powerDown() {
    writeRegister(ES8388_CHIPPOWER, 0xFF); // 關閉所有電源
    writeRegister(ES8388_ADCPOWER, 0xFF);  // 關閉 ADC
    writeRegister(ES8388_DACPOWER, 0xC0);  // 關閉 DAC (保留位元需維持)
}

void ES8388::powerUp() {
    writeRegister(ES8388_CHIPPOWER, 0x00);   // 正常電源
    writeRegister(ES8388_DACPOWER, 0x3C);    // 開啟 DAC 電源 (ROUT1/2, LOUT1/2 enabled)
    writeRegister(ES8388_ADCPOWER, 0x00);    // 開啟 ADC 電源
    writeRegister(ES8388_ANAVOLMANAG, 0x7C); // 類比電源設定
}

void ES8388::standby() {
    // 進入低功耗模式但保留暫存器設定
    writeRegister(ES8388_CHIPPOWER, 0x00); 
    writeRegister(ES8388_ADCPOWER, 0xFF); // 僅關閉 ADC
    writeRegister(ES8388_DACPOWER, 0xC0); // 僅關閉 DAC
}

void ES8388::setVolume(uint8_t volume) {
    if (volume > 100) volume = 100;
    
    // 類比輸出音量控制 (Reg 46-49)
    // 範圍：0x00 (-30dB) 到 0x1E (0dB)
    // 映射 0-100 到 0x00-0x1E (0-30)
    
    uint8_t analog_vol = volume * 30 / 100;
    
    writeRegister(ES8388_DACCONTROL24, analog_vol); // LOUT1 音量
    writeRegister(ES8388_DACCONTROL25, analog_vol); // ROUT1 音量
    writeRegister(ES8388_DACCONTROL26, analog_vol); // LOUT2 音量
    writeRegister(ES8388_DACCONTROL27, analog_vol); // ROUT2 音量
}

void ES8388::mute(bool enable) {
    muteDAC(enable);
    muteADC(enable);
}

void ES8388::muteDAC(bool enable) {
    uint8_t reg = readRegister(ES8388_DACCONTROL3);
    if (enable) {
        reg |= 0x04; // 設定靜音位元
    } else {
        reg &= ~0x04; // 清除靜音位元
    }
    writeRegister(ES8388_DACCONTROL3, reg);
}

void ES8388::muteADC(bool enable) {
    // ADC 靜音在 ADC Control 3 (0x0B) Bit 2
    uint8_t reg = readRegister(ES8388_ADCCONTROL3);
    if (enable) {
        reg |= 0x04; // 數位靜音
    } else {
        reg &= ~0x04;
    }
    writeRegister(ES8388_ADCCONTROL3, reg);
}

void ES8388::setInput(ES8388_InputPath input) {
    // ADC 輸入選擇 (Reg 0x0A)
    // Bits 7-6: LINSEL (左聲道輸入選擇)
    // Bits 5-4: RINSEL (右聲道輸入選擇)
    // 差分輸入: Bits 3-0 (DSel)
    
    uint8_t reg = 0x00;
    switch(input) {
        case ES8388_MIC1: reg = 0x00; break; // MIC1
        case ES8388_MIC2: reg = 0x50; break; // MIC2
        case ES8388_LINE1: reg = 0xA0; break; // LINE1
        case ES8388_LINE2: reg = 0xF0; break; // LINE2
        case ES8388_DIFF_MIC1: reg = 0x0C; break; // 差分 MIC1
        case ES8388_DIFF_MIC2: reg = 0x5C; break; // 差分 MIC2
        default: reg = 0x00; break;
    }
    writeRegister(ES8388_ADCCONTROL2, reg);
}

void ES8388::setInput(uint8_t input) {
    // 相容舊 API
    if (input == 0) setInput(ES8388_MIC1);
    else if (input == 1) setInput(ES8388_MIC2);
    else if (input == 2) setInput(ES8388_LINE1);
    else if (input == 3) setInput(ES8388_LINE2);
    else setInput(ES8388_MIC1);
}

void ES8388::setOutput(uint8_t output) {
    // DAC 電源控制 (Reg 0x04)
    // 根據 header 定義映射到位元
    // LOUT1=0x01 -> Bit 4 (0x10)
    // LOUT2=0x02 -> Bit 2 (0x04)
    // ROUT1=0x04 -> Bit 3 (0x08)
    // ROUT2=0x08 -> Bit 1 (0x02)

    uint8_t reg = readRegister(ES8388_DACPOWER) & 0xC1; // 保留 bit 7,6,0，清除 5-1
    if (output & 0x01) reg |= 0x10; // LOUT1
    if (output & 0x02) reg |= 0x04; // LOUT2
    if (output & 0x04) reg |= 0x08; // ROUT1
    if (output & 0x08) reg |= 0x02; // ROUT2
    
    writeRegister(ES8388_DACPOWER, reg);
}

void ES8388::setOutputMixer(bool left_mix, bool right_mix) {
    // 簡單啟用/禁用 DAC 到 Mixer 的路徑
    // DAC Control 17 (0x27): LD2LO (Bit 7), LD2RO (Bit 4) ...
    // 通常: LD2LM (Left DAC to Left Mixer) = Bit 7
    //      RD2RM (Right DAC to Right Mixer) = Bit 7 of Reg 0x2A
    
    uint8_t l = readRegister(ES8388_DACCONTROL17);
    if (left_mix) l |= 0x80; else l &= ~0x80;
    writeRegister(ES8388_DACCONTROL17, l);
    
    uint8_t r = readRegister(ES8388_DACCONTROL20);
    if (right_mix) r |= 0x80; else r &= ~0x80;
    writeRegister(ES8388_DACCONTROL20, r);
}

void ES8388::enableAnalogBypass(bool enable) {
    // 啟用類比輸入直接輸出到 Mixer (監聽功能)
    // DAC Control 17 (0x27): Bit 6 LI2LO (Left Input to Left Output Mixer)
    // DAC Control 20 (0x2A): Bit 6 RI2RO (Right Input to Right Output Mixer)
    
    uint8_t l = readRegister(ES8388_DACCONTROL17);
    if (enable) l |= 0x40; else l &= ~0x40;
    writeRegister(ES8388_DACCONTROL17, l);
    
    uint8_t r = readRegister(ES8388_DACCONTROL20);
    if (enable) r |= 0x40; else r &= ~0x40;
    writeRegister(ES8388_DACCONTROL20, r);
}

void ES8388::config(uint8_t bits, uint32_t sampleRate) {
    setBitsPerSample(bits);
    // 採樣率設定通常涉及 MCLK 分頻，在 Slave 模式下由外部時鐘決定，
    // 但某些濾波器設定可能需要知道採樣率。暫時保留介面。
}

void ES8388::setBitsPerSample(uint8_t bits) {
    // ADC Control 4 (0x0C): Bits 4-2 (字長)
    uint8_t adc_bits = 0;
    switch(bits) {
        case 16: adc_bits = 0x0C; break; // 16 bits
        case 20: adc_bits = 0x04; break; // 20 bits
        case 24: adc_bits = 0x00; break; // 24 bits
        case 32: adc_bits = 0x10; break; // 32 bits
        default: adc_bits = 0x0C; break;
    }
    uint8_t reg = readRegister(ES8388_ADCCONTROL4);
    reg &= 0xE3; // 清除 bits 4-2
    reg |= adc_bits;
    writeRegister(ES8388_ADCCONTROL4, reg);

    // DAC Control 1 (0x17): Bits 5-3 (字長)
    uint8_t dac_bits = 0;
    switch(bits) {
        case 16: dac_bits = 0x18; break; // 16 bits
        case 20: dac_bits = 0x08; break; // 20 bits
        case 24: dac_bits = 0x00; break; // 24 bits
        case 32: dac_bits = 0x20; break; // 32 bits
        default: dac_bits = 0x18; break;
    }
    reg = readRegister(ES8388_DACCONTROL1);
    reg &= 0xC7; // 清除 bits 5-3
    reg |= dac_bits;
    writeRegister(ES8388_DACCONTROL1, reg);
}

void ES8388::setI2SFormat(ES8388_I2S_Fmt fmt) {
    uint8_t val = 0;
    switch(fmt) {
        case ES8388_I2S_NORMAL: val = 0x00; break; // 00
        case ES8388_I2S_LEFT:   val = 0x01; break; // 01
        case ES8388_I2S_RIGHT:  val = 0x03; break; // 11 (注意：某些版本是 10 或 11，datasheet 差異)
        case ES8388_I2S_DSP:    val = 0x03; break; // DSP/PCM
        default: val = 0x00; break;
    }
    
    // ADC Control 4 (0x0C): Bits 1-0
    uint8_t reg = readRegister(ES8388_ADCCONTROL4);
    reg &= 0xFC;
    reg |= val;
    writeRegister(ES8388_ADCCONTROL4, reg);
    
    // DAC Control 1 (0x17): Bits 2-1
    reg = readRegister(ES8388_DACCONTROL1);
    reg &= 0xF9;
    reg |= (val << 1);
    writeRegister(ES8388_DACCONTROL1, reg);
}

void ES8388::setMasterMode(bool enable) {
    // Reg 0x08: Master Mode Control
    // Bit 7: MSC (Master/Slave Mode), 1=Master, 0=Slave
    uint8_t reg = readRegister(ES8388_MASTERMODE);
    if (enable) reg |= 0x80;
    else reg &= ~0x80;
    writeRegister(ES8388_MASTERMODE, reg);
}

void ES8388::setMicGain(uint8_t gainDb) {
    if (gainDb > 24) gainDb = 24;
    uint8_t gain = gainDb / 3;
    // 暫存器 0x09: Bits 7:4=左聲道增益, 3:0=右聲道增益
    uint8_t val = (gain << 4) | gain;
    writeRegister(ES8388_ADCCONTROL1, val);
}

void ES8388::setALC(bool enable, uint8_t maxGain, uint8_t minGain, uint8_t target, uint8_t holdTime, uint8_t decayTime, uint8_t attackTime) {
    // Reg 0x12 (18) - ADC Control 10: 啟用, Max Gain
    // Bit 7-6: ALC 模式 (00=關閉, 11=ALC), Bit 5-3: Max Gain
    uint8_t reg12 = (enable ? 0xC0 : 0x00) | ((maxGain & 0x07) << 3);
    writeRegister(ES8388_ADCCONTROL10, reg12);
    
    // Reg 0x13 (19) - ADC Control 11: Min Gain
    writeRegister(ES8388_ADCCONTROL11, minGain);
    
    // Reg 0x14 (20) - ADC Control 12: Target, Hold
    // Bit 7-4: 目標電平, Bit 3-0: 保持時間
    uint8_t reg14 = ((target & 0x0F) << 4) | (holdTime & 0x0F);
    writeRegister(ES8388_ADCCONTROL12, reg14);
    
    // Reg 0x15 (21) - ADC Control 13: Decay, Attack
    // Bit 7-4: 衰減時間, Bit 3-0: 啟動時間
    uint8_t reg15 = ((decayTime & 0x0F) << 4) | (attackTime & 0x0F);
    writeRegister(ES8388_ADCCONTROL13, reg15);
}

void ES8388::setNoiseGate(uint8_t threshold) {
    // Reg 0x16 (22) - ADC Control 14: 噪聲門檻
    // Bit 7-5: 門檻值
    // Bit 0: 啟用
    
    uint8_t val = 0;
    if (threshold > 0) {
        val = 0x01 | ((threshold & 0x1F) << 3);
    }
    writeRegister(ES8388_ADCCONTROL14, val);
}

void ES8388::setHPF(bool enable, uint8_t cutoff) {
    // Reg 0x0E (14) - ADC Control 6
    // Bit 7-6: HPF 截止頻率
    // Bit 4: HPF 啟用 (1=啟用, 0=停用)
    
    uint8_t reg = cutoff << 6;
    if (enable) reg |= 0x10; // 啟用 HPF
    writeRegister(ES8388_ADCCONTROL6, reg);
}

void ES8388::setDeemphasis(bool enable) {
    // Reg 0x19 (25) - DAC Control 3
    // Bit 5-4: 去加重 (00=關閉, 01=32k, 10=44.1k, 11=48k)
    // 這裡預設使用 44.1kHz (10)
    uint8_t reg = readRegister(ES8388_DACCONTROL3);
    reg &= 0xCF; // 清除 bit 5-4
    if (enable) reg |= 0x20; // 設定為 10 (44.1k)
    writeRegister(ES8388_DACCONTROL3, reg);
}

void ES8388::setVolumeRaw(uint8_t vol_l, uint8_t vol_r) {
    writeRegister(ES8388_DACCONTROL4, vol_l);
    writeRegister(ES8388_DACCONTROL5, vol_r);
}

void ES8388::writeRegister(uint8_t reg, uint8_t value) {
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->write(value);
    _wire->endTransmission();
}

uint8_t ES8388::readRegister(uint8_t reg) {
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->endTransmission(false); // 重新啟動
    _wire->requestFrom(_address, (uint8_t)1);
    return _wire->read();
}
