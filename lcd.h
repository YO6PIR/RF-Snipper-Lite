#ifndef RF_SNIPPER_LCD_H
#define RF_SNIPPER_LCD_H

#include <stdint.h>

void lcdInit();
void lcdCommand(uint8_t command);
void lcdData(uint8_t data);
void lcdSetCursor(uint8_t column, uint8_t row);
void lcdClear();
void lcdWriteChar(uint8_t value);
void lcdWriteString(const char *text);
void lcdDefineChar(uint8_t code, const uint8_t character[8]);
void lcdDefineChar_P(uint8_t code, const uint8_t character[8]);

#endif
