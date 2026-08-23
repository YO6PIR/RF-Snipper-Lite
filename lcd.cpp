#include "lcd.h"

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

// Original CodeVision LCD wiring: RS=PD0, RW=PB6, EN=PD1, D4..D7=PD4..PD7.
static const uint8_t LCD_RS = _BV(PD0);
static const uint8_t LCD_EN = _BV(PD1);
static const uint8_t LCD_RW = _BV(PB6);

static void lcdWriteNibble(uint8_t nibble) {
  PORTD = (PORTD & 0x0F) | ((nibble & 0x0F) << 4);
  PORTD |= LCD_EN;
  _delay_us(1);
  PORTD &= ~LCD_EN;
  _delay_us(40);
}

static void lcdWrite(uint8_t value, bool data) {
  if (data) PORTD |= LCD_RS; else PORTD &= ~LCD_RS;
  PORTB &= ~LCD_RW;
  lcdWriteNibble(value >> 4);
  lcdWriteNibble(value);
}

void lcdInit() {
  DDRD |= LCD_RS | LCD_EN | _BV(PD4) | _BV(PD5) | _BV(PD6) | _BV(PD7);
  DDRB |= LCD_RW;
  PORTB &= ~LCD_RW;
  PORTD &= ~(LCD_RS | LCD_EN);
  _delay_ms(15);
  lcdWriteNibble(0x03); _delay_ms(5);
  lcdWriteNibble(0x03); _delay_us(150);
  lcdWriteNibble(0x03); _delay_us(150);
  lcdWriteNibble(0x02);
  lcdCommand(0x28); // 4-bit, 2-line, 5x8 character font
  lcdCommand(0x08);
  lcdClear();
  lcdCommand(0x06);
  lcdCommand(0x0C);
}

void lcdCommand(uint8_t command) {
  lcdWrite(command, false);
  if (command == 0x01 || command == 0x02) _delay_ms(2);
}

void lcdData(uint8_t data) { lcdWrite(data, true); }

void lcdSetCursor(uint8_t column, uint8_t row) {
  lcdCommand(0x80 | (row ? 0x40 : 0x00) | column);
}

void lcdClear() { lcdCommand(0x01); }
void lcdWriteChar(uint8_t value) { lcdData(value); }

void lcdWriteString(const char *text) {
  while (*text) lcdWriteChar(*text++);
}

void lcdDefineChar(uint8_t code, const uint8_t character[8]) {
  lcdCommand(0x40 | ((code & 0x07) << 3));
  for (uint8_t i = 0; i < 8; ++i) lcdData(character[i]);
}

void lcdDefineChar_P(uint8_t code, const uint8_t character[8]) {
  lcdCommand(0x40 | ((code & 0x07) << 3));
  for (uint8_t i = 0; i < 8; ++i) lcdData(pgm_read_byte(character + i));
}
