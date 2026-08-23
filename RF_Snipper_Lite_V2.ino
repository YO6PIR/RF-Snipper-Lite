/*
 * RF Snipper Lite V2 -- baseline avr-gcc/Arduino IDE port.
 *
 * Select an ATmega8/ATmega8A board definition configured for an 8 MHz clock.
 * The Arduino core only supplies program startup; peripheral access here is
 * deliberately through AVR registers, as in the original CodeVision project.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

#include "lcd.h"
#include "micrograph.h"
#include "si5351.h"

#if F_CPU != 8000000UL
#error "RF Snipper Lite V2 requires an ATmega8 clocked at 8 MHz. Select/configure an 8 MHz board."
#endif

#define ON 1
#define OFF 0

#define SWEEP_POINT_COUNT 24
#define RF_SETTLE_MS 10
#define RF_MIN_HZ 1000000UL
#define RF_MAX_HZ 50000000UL

#define BUTTON_PRESSED() (!(PINC & _BV(PC2)))
#define ADC_CHANNEL_VFWD 0
#define ADC_CHANNEL_VREW 1

volatile uint8_t old_AB = 0;
volatile uint8_t new_AB = 0;

uint8_t graphLevels[SWEEP_POINT_COUNT];
uint16_t graphSWR[SWEEP_POINT_COUNT];
bool graphDataValid;
uint16_t minSWR;
uint8_t minSWRIndex;
uint32_t minSWRFrequency;
bool minSWRValid;
static uint16_t savedNormalSWR;
static bool savedNormalSWRValid;
int16_t Temp_swr = 0;
uint8_t Plotter = 4;
uint16_t A, B, A_old, H1, H2;
uint8_t Byte1;
int32_t Rotary_encoder = 4;
bool continuousScan;
volatile int8_t continuousCenterDelta;
// The first of the 24 RF points, in Hz.  Every following point is exactly one
// selected STEP away from it.
static int32_t normalWindowStartHz;
uint32_t Freq;
uint32_t Adjust_frecvency;
static uint32_t centerHz;
uint32_t Index_old = 1400;
uint8_t Division = 1;
int32_t F_min = 1380, F_max;
uint32_t high, low;
float result;

const uint16_t step_values_khz[7] = {5, 10, 25, 50, 100, 250, 500};
const char *const step_strings[7] = {"5", "10", "25", "50", "100", "250", "500"};
bool Direction;
uint8_t Status = 1;
bool new_bit1, old_bit0;
bool move = false;

static uint32_t stepHz()
{
  return (uint32_t)step_values_khz[Division] * 1000UL;
}

ISR(INT0_vect)
{
  old_AB = new_AB;
  new_AB = (PIND & 0x0C) >> 2;
  new_bit1 = new_AB & (1 << 1);
  old_bit0 = old_AB & (1 << 0);

  if (new_AB != old_AB)
  {
    Direction = new_bit1 ^ old_bit0;
    if (continuousScan)
    {
      if (Direction)
      {
        if (continuousCenterDelta < 127)
          continuousCenterDelta++;
      }
      else
      {
        if (continuousCenterDelta > -127)
          continuousCenterDelta--;
      }
    }
    else if (Direction)
    {
      if (Status == 2)
        Rotary_encoder += step_values_khz[Division];
      else
        Rotary_encoder++;
    }
    else
    {
      if (Status == 2)
        Rotary_encoder -= step_values_khz[Division];
      else
        Rotary_encoder--;
    }
    move = ON;
  }
}

#define ADC_VREF_TYPE (_BV(REFS1) | _BV(REFS0))

uint16_t read_adc(uint8_t adc_input)
{
  ADMUX = adc_input | ADC_VREF_TYPE;
  _delay_us(10);
  ADCSRA |= _BV(ADSC);
  while ((ADCSRA & _BV(ADIF)) == 0)
  {
  }
  ADCSRA |= _BV(ADIF);
  return ADCW;
}

uint16_t Vfwd;
uint16_t Vrew;
float swr;
float gamma;

float swr_calculate(int fwd, int rfl)
{
  int val_fwd = fwd;
  int val_rfl = rfl;
  if (val_rfl > val_fwd)
    val_rfl = val_fwd;
  if (val_fwd == 0)
  {
    gamma = 0;
    swr = 999;
    return swr;
  }
  gamma = (float)val_rfl / (float)val_fwd;
  swr = (1 + gamma) / (1 - gamma);
  return swr;
}

float swr_read()
{
  int Vfwd = read_adc(1);
  int Vrew = read_adc(0);
  return swr_calculate(Vfwd, Vrew);
}

void Samples()
{
  Vfwd = read_adc(1);
  Vrew = read_adc(0);
  if (Vrew > Vfwd)
    Vrew = Vfwd;
  if (Vfwd == 0)
  {
    gamma = 0;
    swr = 999;
    Temp_swr = 0;
    return;
  }
  gamma = (float)Vrew / (float)Vfwd;
  swr = (1 + gamma) * 100 / (1 - gamma);
  if (swr > 999)
    swr = 999;
  if (swr > 100)
    Temp_swr = (swr / 25) - 4;
  else
    Temp_swr = 0;
  if (Temp_swr > 16)
    Temp_swr = 16;
}

static uint8_t rfSWRToMicroGraphLevel(int16_t graphValue)
{
  if (graphValue <= 0)
    return 0;
  if (graphValue >= 16)
    return 8;
  return (uint8_t)((graphValue + 1) >> 1);
}

void Display(int16_t tmp)
{
  if (tmp / 1000)
    lcdWriteChar(tmp / 1000 + 0x30);
  else
    lcdWriteString(" ");
  lcdWriteChar((tmp / 100) % 10 + 0x30);
  lcdWriteString(",");
  lcdWriteChar((tmp / 10) % 10 + 0x30);
  lcdWriteChar(tmp % 10 + 0x30);
}

bool SW;
uint8_t pas = 0;
static bool continuousGraphStarted;
static bool finalScanPending;
static bool continuousStopReady;
static bool stopPressConsumed;
static bool buttonDown;
static bool longPressFired;
static uint32_t buttonPressTime;
static uint32_t buttonClockMs;

static void findMinimumSWR();
static void invalidateMarkerText();
static void drawMarkerText();

#define LONG_PRESS_MS 1000UL

static void buttonTimerInit()
{
  TCNT2 = 0;
  TIFR = _BV(TOV2);
  TCCR2 = _BV(CS22) | _BV(CS21); // Timer2 /256: one overflow every 8.192 ms.
}

static void buttonTimerPoll()
{
  if (TIFR & _BV(TOV2))
  {
    TIFR = _BV(TOV2);
    buttonClockMs += 8;
  }
}

static bool continuousStopRequested()
{
  if (!continuousScan)
    return false;

  if (!BUTTON_PRESSED())
  {
    continuousStopReady = true;
    buttonDown = false;
    return false;
  }

  if (continuousStopReady)
  {
    continuousScan = false;
    stopPressConsumed = true;
    return true;
  }
  return false;
}

static void handleButton()
{
  buttonTimerPoll();
  bool pressed = BUTTON_PRESSED();

  if (continuousScan)
  {
    if (!pressed)
    {
      continuousStopReady = true;
      buttonDown = false;
    }
    return;
  }

  if (stopPressConsumed)
  {
    if (!pressed)
      stopPressConsumed = false;
    return;
  }

  if (pressed)
  {
    if (!buttonDown)
    {
      buttonDown = true;
      longPressFired = false;
      buttonPressTime = buttonClockMs;
    }
    else if (!longPressFired && Status == 1 &&
             (uint32_t)(buttonClockMs - buttonPressTime) >= LONG_PRESS_MS)
    {
      longPressFired = true;
      continuousScan = true;
      continuousCenterDelta = 0;
      continuousGraphStarted = false;
      continuousStopReady = false;
    }
  }
  else if (buttonDown)
  {
    if (!longPressFired)
      Taste();
    buttonDown = false;
    longPressFired = false;
  }
}

static uint32_t sweepPointFrequencyHz(uint8_t point)
{
  return (uint32_t)(normalWindowStartHz + (int32_t)stepHz() * point);
}

static void resetNormalSweepWindow()
{
  int32_t spanHz = (int32_t)stepHz() * (SWEEP_POINT_COUNT - 1);
  int32_t selectedCenterHz = (int32_t)centerHz;
  int32_t halfSpanHz = spanHz / 2;

  if (selectedCenterHz < (int32_t)RF_MIN_HZ + halfSpanHz)
    selectedCenterHz = (int32_t)RF_MIN_HZ + halfSpanHz;
  if (selectedCenterHz > (int32_t)RF_MAX_HZ - halfSpanHz)
    selectedCenterHz = (int32_t)RF_MAX_HZ - halfSpanHz;
  centerHz = selectedCenterHz;
  normalWindowStartHz = selectedCenterHz - halfSpanHz;
  Freq = centerHz / 10000UL;
  F_min = normalWindowStartHz / 10000L;
  F_max = (normalWindowStartHz + spanHz) / 10000L;
}

static void panNormalSweepWindow(int8_t direction)
{
  int32_t spanHz = (int32_t)stepHz() * (SWEEP_POINT_COUNT - 1);
  int32_t shiftedStart = normalWindowStartHz +
                          (direction > 0 ? (int32_t)stepHz() : -(int32_t)stepHz());

  if (shiftedStart < (int32_t)RF_MIN_HZ ||
      shiftedStart + spanHz > (int32_t)RF_MAX_HZ)
    return;

  normalWindowStartHz = shiftedStart;
  centerHz = normalWindowStartHz + spanHz / 2;
  F_min = normalWindowStartHz / 10000L;
  F_max = (normalWindowStartHz + spanHz) / 10000L;

  microGraphDraw(graphLevels, 255);
  if (direction > 0)
  {
    for (uint8_t point = 0; point < SWEEP_POINT_COUNT - 1; ++point)
    {
      graphSWR[point] = graphSWR[point + 1];
      graphLevels[point] = graphLevels[point + 1];
    }
  }
  else
  {
    for (uint8_t point = SWEEP_POINT_COUNT - 1; point > 0; --point)
    {
      graphSWR[point] = graphSWR[point - 1];
      graphLevels[point] = graphLevels[point - 1];
    }
  }

  uint8_t newPoint = direction > 0 ? SWEEP_POINT_COUNT - 1 : 0;
  uint32_t frequencyHz = sweepPointFrequencyHz(newPoint);
  si5351aSetFrequency(frequencyHz);
  _delay_ms(RF_SETTLE_MS);
  Samples();
  graphSWR[newPoint] = swr < 0 ? 0 : (uint16_t)swr;
  graphLevels[newPoint] = rfSWRToMicroGraphLevel(Temp_swr);

  // Keep menu state aligned with the moved RF window.  The display itself
  // continues to use the exact scaled window through sweepPointFrequencyHz().
  Adjust_frecvency = sweepPointFrequencyHz(SWEEP_POINT_COUNT / 2) / 10000UL;
  findMinimumSWR();
  move = OFF;
  invalidateMarkerText();
  microGraphDraw(graphLevels, Plotter);
  drawMarkerText();
}

static void findMinimumSWR()
{
  minSWR = 999;
  minSWRIndex = 0;
  minSWRFrequency = 0;
  minSWRValid = false;

  for (uint8_t point = 0; point < SWEEP_POINT_COUNT; ++point)
  {
    uint16_t value = graphSWR[point];
    if (value < 999 && (!minSWRValid || value < minSWR))
    {
      minSWR = value;
      minSWRIndex = point;
      minSWRValid = true;
    }
  }

  if (minSWRValid)
    minSWRFrequency = sweepPointFrequencyHz(minSWRIndex);
}

static char previousTopText[16];
static char previousBottomText[16];
static bool markerTextValid;

static void invalidateMarkerText()
{
  markerTextValid = false;
}

static void drawMarkerText()
{
  char top[16];
  char bottom[16];
  for (uint8_t i = 0; i < 16; ++i)
  {
    top[i] = ' ';
    bottom[i] = ' ';
  }

  if (continuousScan)
  {
    top[0] = 'S';
    top[1] = 'c';
    top[2] = 'a';
    top[3] = 'n';
    top[4] = 'i';
    top[5] = 'n';
    top[6] = 'g';
  }
  else
  {
    uint16_t swrDisplay = graphDataValid ? graphSWR[Plotter] : savedNormalSWR;
    top[0] = 'S';
    top[1] = 'W';
    top[2] = 'R';
    top[3] = (swrDisplay >= 999 ||
              (!graphDataValid && !savedNormalSWRValid))
                 ? '-'
                 : (swrDisplay / 100) + '0';
    if (swrDisplay >= 999 || (!graphDataValid && !savedNormalSWRValid))
    {
      top[4] = '-';
      top[5] = '-';
      top[6] = '-';
    }
    else
    {
      top[4] = '.';
      top[5] = ((swrDisplay / 10) % 10) + '0';
      top[6] = (swrDisplay % 10) + '0';
    }
    top[7] = ' ';
  }

  uint32_t displayHz = sweepPointFrequencyHz(Plotter);
  if (continuousScan && minSWRValid)
  {
    displayHz = minSWRFrequency;
    uint16_t wholeMHz = displayHz / 1000000UL;
    uint8_t decimalHundredths = (displayHz % 1000000UL) / 10000UL;
    bottom[0] = wholeMHz >= 10 ? (wholeMHz / 10) + '0' : ' ';
    bottom[1] = (wholeMHz % 10) + '0';
    bottom[2] = '.';
    bottom[3] = (decimalHundredths / 10) + '0';
    bottom[4] = (decimalHundredths % 10) + '0';
    bottom[5] = 'M';
    bottom[6] = 'H';
    bottom[7] = 'z';
    bottom[8] = ' ';
    bottom[9] = 'M';
    bottom[10] = 'i';
    bottom[11] = 'n';
    bottom[12] = (minSWR / 100) + '0';
    bottom[13] = '.';
    bottom[14] = ((minSWR / 10) % 10) + '0';
    bottom[15] = (minSWR % 10) + '0';
  }
  else
  {
    uint16_t wholeMHz = displayHz / 1000000UL;
    uint8_t decimalHundredths = (displayHz % 1000000UL) / 10000UL;
    bottom[0] = wholeMHz < 10 ? ' ' : (wholeMHz / 10) + '0';
    bottom[1] = (wholeMHz % 10) + '0';
    bottom[2] = '.';
    bottom[3] = (decimalHundredths / 10) + '0';
    bottom[4] = (decimalHundredths % 10) + '0';
    bottom[5] = 'M';
    bottom[6] = 'H';
    bottom[7] = 'z';
    const char *stepText = step_strings[Division];
    uint8_t column = 9;
    while (*stepText)
      bottom[column++] = *stepText++;
    bottom[column++] = 'K';
    bottom[column++] = '/';
    bottom[column] = 'd';
  }

  for (uint8_t column = 0; column < 16; ++column)
  {
    if (column < 8 &&
        (!markerTextValid || top[column] != previousTopText[column]))
    {
      lcdSetCursor(column, 0);
      lcdWriteChar(top[column]);
      previousTopText[column] = top[column];
    }
    if (!markerTextValid || bottom[column] != previousBottomText[column])
    {
      lcdSetCursor(column, 1);
      lcdWriteChar(bottom[column]);
      previousBottomText[column] = bottom[column];
    }
  }
  markerTextValid = true;
}

// In Continuous Scan the encoder moves the RF window, not the 24-point
// marker.  Consume all ISR steps atomically and use the same CENTER step as
// the normal CENTER adjustment: one current DIV value per encoder step.
static bool applyContinuousCenterChange()
{
  int8_t delta;
  uint8_t savedSreg = SREG;
  cli();
  delta = continuousCenterDelta;
  continuousCenterDelta = 0;
  SREG = savedSreg;

  if (delta == 0)
    return false;

  int32_t spanHz = (int32_t)stepHz() * (SWEEP_POINT_COUNT - 1);
  int32_t newStartHz = normalWindowStartHz +
                       ((int32_t)delta * (int32_t)stepHz());
  if (newStartHz < (int32_t)RF_MIN_HZ ||
      newStartHz + spanHz > (int32_t)RF_MAX_HZ)
    return false;
  if (newStartHz == normalWindowStartHz)
    return false;

  normalWindowStartHz = newStartHz;
  centerHz = normalWindowStartHz + spanHz / 2;
  F_min = normalWindowStartHz / 10000L;
  F_max = (normalWindowStartHz + spanHz) / 10000L;
  Adjust_frecvency = sweepPointFrequencyHz(SWEEP_POINT_COUNT / 2) / 10000UL;
  Freq = Adjust_frecvency;
  Plotter = SWEEP_POINT_COUNT / 2;
  graphDataValid = false;
  continuousGraphStarted = false;
  move = OFF;
  invalidateMarkerText();
  drawMarkerText();
  return true;
}

void Swap_frequency()
{
  if (continuousScan)
  {
    savedNormalSWR = graphSWR[Plotter];
    savedNormalSWRValid = graphDataValid;
  }
  else
  {
    savedNormalSWRValid = false;
    resetNormalSweepWindow();
  }
  // A normal/result sweep starts from an empty graph.  Continuous mode
  // clears only its first frame, then keeps both the old pixels and the
  // CGRAM cache so each new RF point overwrites its previous value.
  if (!continuousScan || !continuousGraphStarted)
  {
    microGraphClear();
    for (uint8_t point = 0; point < SWEEP_POINT_COUNT; ++point)
      graphLevels[point] = 0;
    graphDataValid = false;
    drawMarkerText();
    if (continuousScan)
      continuousGraphStarted = true;
  }

  for (pas = 0; pas < SWEEP_POINT_COUNT; pas++)
  {
    uint32_t frequencyHz = sweepPointFrequencyHz(pas);
    Freq = frequencyHz / 10000UL;
    si5351aSetFrequency(frequencyHz);
    _delay_ms(RF_SETTLE_MS);
    Samples();
    if (continuousScan && applyContinuousCenterChange())
      return;

    // Keep the preceding scan cursor visible while this RF point settles and
    // is measured.  Only now restore that glyph before drawing the new one.
    microGraphDraw(graphLevels, 255);

    graphSWR[pas] = swr < 0 ? 0 : (uint16_t)swr;
    graphLevels[pas] = rfSWRToMicroGraphLevel(Temp_swr);
    // Draw the newly measured bar and overlay the existing XOR cursor at
    // exactly this 24-point sweep position.
    microGraphDraw(graphLevels, pas);
    if (continuousStopRequested())
    {
      // Do not carry the temporary continuous-scan cursor into final mode.
      microGraphDraw(graphLevels, 255);
      continuousGraphStarted = false;
      findMinimumSWR();
      if (minSWRValid)
        centerHz = minSWRFrequency - stepHz() / 2;
      Plotter = SWEEP_POINT_COUNT / 2;
      Rotary_encoder = Plotter;
      finalScanPending = true;
      SW = OFF;
      return;
    }
  }

  findMinimumSWR();
  Plotter = SWEEP_POINT_COUNT / 2;
  Rotary_encoder = Plotter;
  graphDataValid = true;
  if (!continuousScan)
    microGraphDraw(graphLevels, Plotter);
  drawMarkerText();
  SW = OFF;
}

void Taste()
{
  Status++;
  if (Status > 3)
  {
    Status = 1;
    resetNormalSweepWindow();
    SW = ON;
    lcdClear();
  }
  while (BUTTON_PRESSED())
  {
  }
  switch (Status)
  {
  case 1:
    Plotter = SWEEP_POINT_COUNT / 2;
    Rotary_encoder = Plotter;
    break;
  case 2:
    centerHz = sweepPointFrequencyHz(Plotter);
    Adjust_frecvency = centerHz / 10000UL;
    Rotary_encoder = centerHz / 1000UL;
    break;
  case 3:
    Rotary_encoder = Division;
    break;
  }
  lcdClear();
  invalidateMarkerText();
}

void Main_display()
{
  if (SW)
    Swap_frequency();
  else
  {
    if (graphDataValid)
      microGraphDraw(graphLevels, Plotter);
    drawMarkerText();
  }
}

void Central_freq()
{
  lcdSetCursor(0, 0);
  lcdWriteString("Center Frecvency");
  centerHz = (uint32_t)Rotary_encoder * 1000UL;
  Freq = centerHz / 10000UL;
  lcdSetCursor(4, 1);
  Display(Freq);
  lcdWriteString(" MHz");
  if (move == ON)
  {
    si5351aSetFrequency(centerHz);
    move = OFF;
  }
}

uint16_t Rotunjire(uint32_t Frecventa)
{
  uint32_t roundedHz = (centerHz / stepHz()) * stepHz();
  (void)Frecventa;
  centerHz = roundedHz;
  Freq = roundedHz / 10000UL;
  return Freq;
}

void Interval_div()
{
  lcdSetCursor(0, 0);
  lcdWriteString("STEP Div. =     ");
  lcdSetCursor(12, 0);
  lcdWriteString(step_strings[Division]);
  lcdWriteString("K");
  Rotunjire(Freq);
  resetNormalSweepWindow();
  lcdSetCursor(0, 1);
  Display(F_min);
  uint8_t dashCount = Division;
  if (dashCount == 0)
    dashCount = 1;
  for (Byte1 = 0; Byte1 < dashCount; Byte1++)
    lcdWriteString("-");
  Display(F_max);
  lcdWriteString("    ");
}

static void hardwareInit()
{
  DDRB = 0;
  PORTB = 0;
  DDRC = 0;
  PORTC = _BV(PC3) | _BV(PC2);
  DDRD = _BV(PD7) | _BV(PD6) | _BV(PD5) | _BV(PD4) | _BV(PD1) | _BV(PD0);
  PORTD = _BV(PD3) | _BV(PD2);

  TCCR0 = 0;
  TCNT0 = 0;
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  ICR1 = 0;
  OCR1A = 0;
  OCR1B = 0;
  ASSR = 0;
  TCCR2 = 0;
  TCNT2 = 0;
  OCR2 = 0;
  TIMSK = 0;
  GICR = _BV(INT0);
  MCUCR = _BV(ISC00);
  GIFR = _BV(INTF0);
  UCSRB = 0;
  ADMUX = _BV(REFS0) | (0x0e << MUX0);
  ADCSRA = _BV(ADEN) | _BV(ADPS2);
  SFIOR = 0;
  SPCR = 0;
  TWCR = _BV(TWEA);
}

void setup()
{
  hardwareInit();
  buttonTimerInit();
  i2cInit();
  si5351aSetFrequency(14000000);
  lcdInit();
  sei();
  lcdSetCursor(0, 0);
  lcdWriteString(" RF-Sniper Lite ");
  lcdSetCursor(0, 1);
  lcdWriteString("YO6PIR ---> 2026");
  _delay_ms(2000);
  lcdClear();
  microGraphInit();

  // Boot defaults: 14.000 MHz center, 100 kHz STEP, one normal scan.
  Freq = 1400;
  centerHz = 14000000UL;
  Adjust_frecvency = Freq;
  Division = 4;
  resetNormalSweepWindow();
  Plotter = SWEEP_POINT_COUNT / 2;
  Rotary_encoder = Plotter;
  graphDataValid = false;
  SW = ON;
}

void loop()
{
  handleButton();
  if (continuousScan)
  {
    Swap_frequency();
    return;
  }
  if (finalScanPending)
  {
    finalScanPending = false;
    SW = ON;
    Swap_frequency();
    return;
  }
  Samples();
  switch (Status)
  {
  case 1:
    if (graphDataValid && Rotary_encoder > (SWEEP_POINT_COUNT - 1))
    {
      Rotary_encoder = SWEEP_POINT_COUNT - 1;
      Plotter = Rotary_encoder;
      panNormalSweepWindow(1);
    }
    else if (graphDataValid && Rotary_encoder < 0)
    {
      Rotary_encoder = 0;
      Plotter = Rotary_encoder;
      panNormalSweepWindow(-1);
    }
    else
    {
      if (Rotary_encoder > (SWEEP_POINT_COUNT - 1))
        Rotary_encoder = SWEEP_POINT_COUNT - 1;
      if (Rotary_encoder < 0)
        Rotary_encoder = 0;
    }
    Plotter = Rotary_encoder;
    Main_display();
    break;
  case 2:
  {
    uint16_t halfSpanKHz = (step_values_khz[Division] *
                            (SWEEP_POINT_COUNT - 1) + 1) / 2;
    Index_old = centerHz / 10000UL;
    if (Rotary_encoder > 50000 - halfSpanKHz)
      Rotary_encoder = 50000 - halfSpanKHz;
    if (Rotary_encoder < 1000 + halfSpanKHz)
      Rotary_encoder = 1000 + halfSpanKHz;
    Adjust_frecvency = Rotary_encoder / 10;
    Central_freq();
    break;
  }
  case 3:
    if (Rotary_encoder > 6)
      Rotary_encoder = 6;
    if (Rotary_encoder < 0)
      Rotary_encoder = 0;
    Division = Rotary_encoder;
    Interval_div();
    break;
  }
}
