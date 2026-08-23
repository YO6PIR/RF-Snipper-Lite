#include "si5351.h"

#include <avr/io.h>

#define SI_CLK0_CONTROL 16
#define SI_SYNTH_PLL_A  26
#define SI_SYNTH_MS_0   42
#define SI_R_DIV_1      0b00000000
#define SI_CLK_SRC_PLL_A 0b00000000
#define SI_PLL_RESET    177
#define XTAL_FREQ       25000000UL

#define I2C_START     0x08
#define I2C_START_RPT 0x10
#define I2C_SLA_W_ACK 0x18
#define I2C_DATA_ACK  0x28
#define I2C_WRITE     0b11000000

static uint8_t i2cStart() {
  TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
  while (!(TWCR & _BV(TWINT))) {}
  return TWSR & 0xF8;
}

static void i2cStop() {
  TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);
  while (TWCR & _BV(TWSTO)) {}
}

static uint8_t i2cByteSend(uint8_t data) {
  TWDR = data;
  TWCR = _BV(TWINT) | _BV(TWEN);
  while (!(TWCR & _BV(TWINT))) {}
  return TWSR & 0xF8;
}

void i2cInit() {
  TWBR = 92;
  TWSR = 0;
  TWDR = 0xFF;
}

uint8_t i2cSendRegister(uint8_t reg, uint8_t data) {
  uint8_t stts = i2cStart();
  if (stts != I2C_START) return 1;
  stts = i2cByteSend(I2C_WRITE);
  if (stts != I2C_SLA_W_ACK) return 2;
  stts = i2cByteSend(reg);
  if (stts != I2C_DATA_ACK) return 3;
  stts = i2cByteSend(data);
  if (stts != I2C_DATA_ACK) return 4;
  i2cStop();
  return 0;
}

static void setupPLL(uint8_t pll, uint8_t mult, uint32_t num, uint32_t denom) {
  uint32_t fraction128 = (uint32_t)(128 * ((float)num / (float)denom));
  uint32_t P1 = fraction128;
  P1 = (uint32_t)(128 * (uint32_t)mult + P1 - 512);
  uint32_t P2 = fraction128;
  P2 = (uint32_t)(128 * num - denom * P2);
  uint32_t P3 = denom;
  i2cSendRegister(pll + 0, (P3 & 0x0000FF00) >> 8); i2cSendRegister(pll + 1, P3 & 0xFF);
  i2cSendRegister(pll + 2, (P1 & 0x00030000) >> 16); i2cSendRegister(pll + 3, (P1 & 0x0000FF00) >> 8);
  i2cSendRegister(pll + 4, P1 & 0xFF); i2cSendRegister(pll + 5, ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16));
  i2cSendRegister(pll + 6, (P2 & 0x0000FF00) >> 8); i2cSendRegister(pll + 7, P2 & 0xFF);
}

static void setupMultisynth(uint8_t synth, uint32_t divider, uint8_t rDiv) {
  uint32_t P1 = 128 * divider - 512;
  uint32_t P2 = 0;
  uint32_t P3 = 1;
  i2cSendRegister(synth + 0, (P3 & 0x0000FF00) >> 8); i2cSendRegister(synth + 1, P3 & 0xFF);
  i2cSendRegister(synth + 2, ((P1 & 0x00030000) >> 16) | rDiv); i2cSendRegister(synth + 3, (P1 & 0x0000FF00) >> 8);
  i2cSendRegister(synth + 4, P1 & 0xFF); i2cSendRegister(synth + 5, ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16));
  i2cSendRegister(synth + 6, (P2 & 0x0000FF00) >> 8); i2cSendRegister(synth + 7, P2 & 0xFF);
}

void si5351aOutputOff(uint8_t clk) { i2cSendRegister(clk, 0x80); }

void si5351aSetFrequency(uint32_t frequency) {
  uint32_t divider = 900000000UL / frequency;
  if (divider % 2) divider--;
  uint32_t pllFreq = divider * frequency;
  uint8_t mult = pllFreq / XTAL_FREQ;
  uint32_t l = pllFreq % XTAL_FREQ;
  float f = l;
  f *= 1048575;
  f /= XTAL_FREQ;
  uint32_t num = f;
  uint32_t denom = 1048575;
  setupPLL(SI_SYNTH_PLL_A, mult, num, denom);
  setupMultisynth(SI_SYNTH_MS_0, divider, SI_R_DIV_1);
  i2cSendRegister(SI_CLK0_CONTROL, 0x4F | SI_CLK_SRC_PLL_A);
}
