#ifndef RF_SNIPPER_MICROGRAPH_H
#define RF_SNIPPER_MICROGRAPH_H

#include <stdint.h>

void microGraphInit();
void microGraphShow();
void microGraphClear();
void microGraphDraw(const uint8_t levels[24], uint8_t marker);

#endif
