#include "ES8388.h"

ES8388::ES8388() {
    _address = ES8388_ADDR;
}

bool ES8388::begin(TwoWire *wire) {
    _wire = wire;
    // _wire->begin(); // Removed: User must initialize Wire before calling begin()
    
    // Check if device is connected
    _wire->beginTransmission(_address);
    if (_wire->endTransmission() != 0) {
        return false;
    }

    // Soft Reset
    writeRegister(ES8388_DACCONTROL3, 0x04); // Mute DAC
    writeRegister(ES8388_CONTROL2, 0x7F);    // Reset all
    writeRegister(ES8388_CHIPPOWER, 0xF0);   // Power down
    delay(10);
    writeRegister(ES8388_CONTROL2, 0x00);    // Release reset
    
    // Power Management
    writeRegister(ES8388_CHIPPOWER, 0x00);   // Normal power
    writeRegister(ES8388_DACPOWER, 0x3C);    // Power up DAC
    writeRegister(ES8388_ADCPOWER, 0x00);    // Power up ADC
    writeRegister(ES8388_ANAVOLMANAG, 0x7C); // Analog power set

    // Slave Mode (Default)
    writeRegister(ES8388_MASTERMODE, 0x00); 

    // DAC Control
    writeRegister(ES8388_DACCONTROL1, 0x18); // DVDD, 16-bit, I2S
    writeRegister(ES8388_DACCONTROL2, 0x02); // DACL/R mixed
    writeRegister(ES8388_DACCONTROL16, 0x00); // 0dB gain
    writeRegister(ES8388_DACCONTROL17, 0x90); // Enable Left DAC to Left Mixer
    writeRegister(ES8388_DACCONTROL20, 0x90); // Enable Right DAC to Right Mixer
    writeRegister(ES8388_DACCONTROL21, 0x80); // DAC DRC enable
    writeRegister(ES8388_DACCONTROL23, 0x00); // 0dB
    
    // Set LOUT/ROUT Output Mixer Volume to 0dB (Max)
    // Register 38 (0x26) - DAC Control 16: DAC Volume Control
    // Register 46 (0x2E) - DAC Control 24: LOUT1 Volume
    // Register 47 (0x2F) - DAC Control 25: ROUT1 Volume
    // Register 48 (0x30) - DAC Control 26: LOUT2 Volume
    // Register 49 (0x31) - DAC Control 27: ROUT2 Volume
    // Value: 0x00 = 0dB (Max), 0x21 = -33dB (Min) for analog output
    
    // Default analog output volume to max (0dB)
    writeRegister(ES8388_DACCONTROL24, 0x1E); // LOUT1 Volume (0x1E is default +0dB roughly in some docs, but 0x00 is max. Let's try 0x00 for max or 0x1E for safe high)
    // Actually, datasheet says:
    // 0x2E-0x31: Analog Output Volume.
    // Range: 0x00 (-30dB) to 0x1E (0dB) ... Wait, let's check datasheet
    // ES8388 Datasheet:
    // Reg 46-49: 
    // 00000 = -30dB
    // ...
    // 11110 = 0dB
    // 11111 = 0dB
    // So 0x1E (30) is 0dB (Max). 
    
    writeRegister(ES8388_DACCONTROL24, 0x1E); // LOUT1 Volume 0dB
    writeRegister(ES8388_DACCONTROL25, 0x1E); // ROUT1 Volume 0dB
    writeRegister(ES8388_DACCONTROL26, 0x1E); // LOUT2 Volume 0dB
    writeRegister(ES8388_DACCONTROL27, 0x1E); // ROUT2 Volume 0dB

    // ADC Control
    writeRegister(ES8388_ADCCONTROL1, 0x88); // MIC gain +24dB
    writeRegister(ES8388_ADCCONTROL2, 0xF0); 
    writeRegister(ES8388_ADCCONTROL3, 0x00);
    writeRegister(ES8388_ADCCONTROL4, 0x0C); // 16-bit, I2S
    writeRegister(ES8388_ADCCONTROL5, 0x02); // 0dB

    setVolume(60); // Default volume
    
    return true;
}

void ES8388::setVolume(uint8_t volume) {
    if (volume > 100) volume = 100;
    
    // Analog Output Volume Control (Reg 46-49)
    // Range: 0x00 (-30dB) to 0x1E (0dB)
    // Map 0-100 to 0x00-0x1E (0-30)
    
    uint8_t analog_vol = volume * 30 / 100;
    
    writeRegister(ES8388_DACCONTROL24, analog_vol); // LOUT1 Volume
    writeRegister(ES8388_DACCONTROL25, analog_vol); // ROUT1 Volume
    writeRegister(ES8388_DACCONTROL26, analog_vol); // LOUT2 Volume
    writeRegister(ES8388_DACCONTROL27, analog_vol); // ROUT2 Volume
}

void ES8388::mute(bool enable) {
    uint8_t reg = readRegister(ES8388_DACCONTROL3);
    if (enable) {
        reg |= 0x04; // Set mute bit
    } else {
        reg &= ~0x04; // Clear mute bit
    }
    writeRegister(ES8388_DACCONTROL3, reg);
}

void ES8388::setInput(uint8_t input) {
    // Simplified input selection
    // 0x00: Lin1/Rin1
    // 0x50: Lin2/Rin2
    // Differential vs Single ended logic is complex, keeping simple
    
    // ADC Input selection
    // Reg 10 (0x0A) - ADCCONTROL2
    // Bits 7-6: LINSEL, Bits 5-4: RINSEL
    
    uint8_t reg = 0x00;
    if (input == ES8388_INPUT_MIC1) {
         reg = 0x00; // LIN1/RIN1
    } else if (input == ES8388_INPUT_MIC2) {
         reg = 0x50; // LIN2/RIN2
    }
    writeRegister(ES8388_ADCCONTROL2, reg);
}

void ES8388::setOutput(uint8_t output) {
    // Power up/down specific outputs
    // Reg 04 (0x04) - DACPOWER
    // Bit 4: Lout1, Bit 3: Rout1, Bit 2: Lout2, Bit 1: Rout2
    
    uint8_t reg = 0x00; // Default all on
    // Logic: 1 = enable (in our API), but register might be 1=enable or 0=enable?
    // Datasheet: 1=Enable.
    
    if (output & ES8388_OUTPUT_LOUT1) reg |= 0x10;
    if (output & ES8388_OUTPUT_ROUT1) reg |= 0x08;
    if (output & ES8388_OUTPUT_LOUT2) reg |= 0x04;
    if (output & ES8388_OUTPUT_ROUT2) reg |= 0x02;
    
    writeRegister(ES8388_DACPOWER, reg);
}

void ES8388::config(uint8_t bits, uint32_t sampleRate) {
    // Configure bit depth
    uint8_t val = 0;
    switch(bits) {
        case 16: val = 0x0C; break; // 16-bit
        case 24: val = 0x00; break; // 24-bit
        case 32: val = 0x04; break; // 32-bit
        default: val = 0x0C; break;
    }
    
    // Set for both ADC and DAC (assuming symmetric)
    // DAC Control 1 (0x17) bits 2:1
    // ADC Control 4 (0x0C) bits 4:2 (Wait, check bits)
    
    // ADC Control 4: 0x0C
    // Bits 4-2: 000=24, 001=20, 010=18, 011=16, 100=32
    uint8_t adc_bits = 0;
    switch(bits) {
        case 16: adc_bits = 0x0C; break; // 011 << 2 = 0x0C
        case 24: adc_bits = 0x00; break;
        case 32: adc_bits = 0x10; break; // 100 << 2 = 0x10
        default: adc_bits = 0x0C; break;
    }
    writeRegister(ES8388_ADCCONTROL4, adc_bits);

    // DAC Control 1: 0x17
    // Bits 5-3: Word Length (000=24, 001=20, 010=18, 011=16, 100=32)
    // Bits 2-1: Format (00=I2S, 01=Left, 10=DSP, 11=Right)
    
    // We assume I2S format (00) for Bits 2-1
    // For 16-bit, we need 011 in Bits 5-3 -> 0001 1000 -> 0x18
    
    uint8_t dac_bits = 0;
    switch(bits) {
        case 16: dac_bits = 0x18; break; // 16-bit
        case 24: dac_bits = 0x00; break; // 24-bit
        case 32: dac_bits = 0x20; break; // 32-bit (100 << 3)
        default: dac_bits = 0x18; break;
    }
    
    uint8_t reg = readRegister(ES8388_DACCONTROL1);
    reg &= ~0x3E; // Clear bits 5-1 (Word length and Format)
    reg |= dac_bits;
    writeRegister(ES8388_DACCONTROL1, reg);
}

void ES8388::setMicGain(uint8_t gainDb) {
    if (gainDb > 24) gainDb = 24;
    uint8_t gain = gainDb / 3;
    // Register 0x09: Bits 7:4=Left Gain, 3:0=Right Gain
    uint8_t val = (gain << 4) | gain;
    writeRegister(ES8388_ADCCONTROL1, val);
}

void ES8388::setHPF(bool enable) {
    // Register 0x0E (14): 0x30 = Enable HPF for both channels
    uint8_t val = enable ? 0x30 : 0x00;
    writeRegister(ES8388_ADCCONTROL6, val);
}

void ES8388::setNoiseGate(uint8_t threshold) {
    if (threshold < 1) threshold = 1;
    if (threshold > 31) threshold = 31;
    // Register 0x15 (ADC Control 13): Bit 7:6=Mute Attenuation (11=Min), Bit 5:0=Threshold
    // Note: Assuming User's Noise Gate threshold maps to this ALC register
    uint8_t val = 0xC0 | (threshold & 0x1F);
    writeRegister(ES8388_ADCCONTROL13, val);
}

void ES8388::setVolumeRaw(uint8_t vol_l, uint8_t vol_r) {
    // User code uses Digital Volume Control (Reg 0x1A, 0x1B)
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
    _wire->endTransmission(false); // Restart
    _wire->requestFrom(_address, (uint8_t)1);
    return _wire->read();
}
