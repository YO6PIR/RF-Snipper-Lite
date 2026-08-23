#include "micrograph.h"

#include "lcd.h"

static uint8_t previousBitmap[8][8];
static uint8_t bitmapValid[8];

static uint8_t clampHeight(uint8_t height) {
  return height > 8 ? 8 : height;
}

static void buildMicroBarChar(uint8_t left, uint8_t center, uint8_t right,
                              uint8_t bitmap[8]) {
  left = clampHeight(left);
  center = clampHeight(center);
  right = clampHeight(right);

  for (uint8_t row = 0; row < 8; ++row) {
    uint8_t value = 0;
    if (row >= 8 - left) value |= 0b10000;
    if (row >= 8 - center) value |= 0b00100;
    if (row >= 8 - right) value |= 0b00001;
    bitmap[row] = value;
  }
}

void microGraphInit() {
  for (uint8_t slot = 0; slot < 8; ++slot) bitmapValid[slot] = 0;
  microGraphShow();
}

void microGraphShow() {
  lcdSetCursor(8, 0);
  for (uint8_t slot = 0; slot < 8; ++slot) lcdWriteChar(slot);
}

void microGraphClear() {
  uint8_t emptyBitmap[8] = {0};

  for (uint8_t slot = 0; slot < 8; ++slot) {
    lcdDefineChar(slot, emptyBitmap);
    for (uint8_t row = 0; row < 8; ++row) previousBitmap[slot][row] = 0;
    bitmapValid[slot] = 1;
  }
  microGraphShow();
}

void microGraphDraw(const uint8_t levels[24], uint8_t marker) {
  uint8_t bitmap[8];

  for (uint8_t slot = 0; slot < 8; ++slot) {
    buildMicroBarChar(levels[slot * 3], levels[slot * 3 + 1],
                      levels[slot * 3 + 2], bitmap);

    if (marker < 24 && marker / 3 == slot) {
      uint8_t cursorBit;
      if ((marker % 3) == 0) cursorBit = 0b10000;
      else if ((marker % 3) == 1) cursorBit = 0b00100;
      else cursorBit = 0b00001;

      // Invert the selected RF microbar on alternating rows for a full-height
      // dotted cursor that remains visible over both bar and background.
      for (uint8_t row = 0; row < 8; ++row) {
        if ((row & 1) == 0) bitmap[row] ^= cursorBit;
      }
    }

    uint8_t changed = !bitmapValid[slot];
    for (uint8_t row = 0; row < 8; ++row) {
      if (bitmap[row] != previousBitmap[slot][row]) changed = 1;
    }

    if (changed) {
      lcdDefineChar(slot, bitmap);
      for (uint8_t row = 0; row < 8; ++row) previousBitmap[slot][row] = bitmap[row];
      bitmapValid[slot] = 1;
    }
  }
}
