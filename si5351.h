#ifndef RF_SNIPPER_SI5351_H
#define RF_SNIPPER_SI5351_H

#include <stdint.h>

void i2cInit();
uint8_t i2cSendRegister(uint8_t reg, uint8_t data);
void si5351aOutputOff(uint8_t clk);
void si5351aSetFrequency(uint32_t frequency);

#endif
