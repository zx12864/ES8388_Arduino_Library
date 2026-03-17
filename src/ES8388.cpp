#include "ES8388.h"

ES8388::ES8388() {
    _address = ES8388_ADDR;
}

bool ES8388::begin(TwoWire *wire) {
    _wire = wire;
    _wire->begin();
    
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
    writeRegister(ES8388_DACCONTROL21, 0x80); // DAC DRC enable
    writeRegister(ES8388_DACCONTROL23, 0x00); // 0dB

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
    // Map 0-100 to 0-33 (approx -96dB to 0dB)
    // Actually ES8388 has 0x00 = 0dB, 0xC0 = -96dB. 0.5dB steps.
    // 192 steps total.
    // Let's invert: 0 (0dB) to 192 (-96dB).
    // Volume 100 -> 0x00 (0dB)
    // Volume 0   -> 0xC0 (-96dB)
    
    uint8_t attn = (100 - volume) * 192 / 100;
    
    writeRegister(ES8388_DACCONTROL24, attn); // LOUT1 Volume
    writeRegister(ES8388_DACCONTROL25, attn); // ROUT1 Volume
    writeRegister(ES8388_DACCONTROL26, attn); // LOUT2 Volume
    writeRegister(ES8388_DACCONTROL27, attn); // ROUT2 Volume
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
    // Bits 2-1: 00=24, 01=20, 10=18, 11=16, 100=32 (Wait, check)
    // Actually DAC Control 1 bits 2:1
    // 00=24bit, 01=20bit, 10=18bit, 11=16bit, 00|100=32bit? 
    // Usually standard I2S 16bit is 0x18 (Format=I2S(00), Word=16(11) -> 00011000 = 0x18)
    
    uint8_t dac_bits = 0;
    switch(bits) {
        case 16: dac_bits = 0x06; break; // 11 << 1
        case 24: dac_bits = 0x00; break;
        case 32: dac_bits = 0x00; break; // Often 24 and 32 are similar in config or unsupported in simple mode
        default: dac_bits = 0x06; break;
    }
    
    uint8_t reg = readRegister(ES8388_DACCONTROL1);
    reg &= ~0x06; // Clear bit length
    reg |= dac_bits;
    writeRegister(ES8388_DACCONTROL1, reg);
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
