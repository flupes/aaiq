#include "graph_samples.h"

#include "Adafruit_GFX.h"
#include "Fonts/ClearSans-Medium-12pt7b.h"
#include "Fonts/ClearSans-Medium-8pt7b.h"
#include "Fonts/Picopixel.h"
#include "epd2in7b.h"

constexpr int16_t kGraphXstart = 2;
constexpr int16_t kGraphYstart = EPD_HEIGHT - 2;

int16_t GraphSamples::ValueToVerticalPixel(int16_t value) {
  return (kGraphYstart -
          kGraphHeight * (value - lowerLimit_) / (upperLimit_ - lowerLimit_));
}

// Allocate 8K of randomness
const size_t kMaxRandomNumbers = 2048;
uint32_t gRandoms[kMaxRandomNumbers];

void GraphSamples::Labels() {
  // char buffer[8];
  blackCanvas_->setTextColor(1);
  blackCanvas_->setTextSize(1);
  blackCanvas_->setTextWrap(false);
  blackCanvas_->setFont(&ClearSans_Medium12pt7b);

  // legend
  uint32 hourPeriod = length_ * period_ / 3600;
  printf("period in hours = %d\n", hourPeriod);
  char msg[8];
  if (hourPeriod < 100) {
    int16_t base = EPD_HEIGHT - 52;
    blackCanvas_->setCursor(EPD_WIDTH - 11, base + 8);
    blackCanvas_->print("h");
    sprintf(msg, "%d", hourPeriod);
    for (size_t c = 0; c < 2; c++) {
      char l = msg[c];
      blackCanvas_->setCursor(EPD_WIDTH - 22, base);
      blackCanvas_->print(l);
      base += 16;
    }
  }

  // upper limit text
  int16_t uBase = EPD_HEIGHT - kGraphHeight + 16;
  blackCanvas_->setCursor(EPD_WIDTH - 27, uBase);
  blackCanvas_->print(upperLimit_ / 100);
  // lower limit text
  int16_t lBase = EPD_HEIGHT - 2;
  bool lowIsZero = false;
  if (lowerLimit_ < 100) {
    blackCanvas_->setCursor(EPD_WIDTH - 20, lBase);
    blackCanvas_->print("0");
    lowIsZero = true;
  } else {
    blackCanvas_->setCursor(EPD_WIDTH - 27, lBase);
    blackCanvas_->print(lowerLimit_ / 100);
  }
  // blackCanvas_->setFont(&Picopixel);
  blackCanvas_->setFont(&ClearSans_Medium8pt7b);
  if (!lowIsZero) {
    blackCanvas_->setCursor(EPD_WIDTH - 16, lBase - 5);
    blackCanvas_->print("00");
  }
  blackCanvas_->setCursor(EPD_WIDTH - 16, uBase - 5);
  blackCanvas_->print("00");
}

void GraphSamples::Background() {
  // Let's have some fun: I do not know how to various shade of
  // grey with dithering, so just draw random dots with various
  // densities! The EPS has a real random generator that can 
  // produce plenty enough of numbers in less than 10ms
  unsigned long start = millis();
  ESP.random((uint8_t*)gRandoms, kMaxRandomNumbers*sizeof(uint32_t));
  unsigned long stop = millis();
  printf("generated %d random bytes in %ldms\n", kMaxRandomNumbers*sizeof(uint32_t),
         stop - start);

  const float densities[] = {0.15, 0.3, 0.5, 0.7, 0.7};
  const int16_t xMin = kGraphXstart + 1;
  const int16_t xMax = kGraphXstart + kGraphWidth - 1;
  const int16_t xRange = xMax - xMin;
  size_t currentDensity = 0;
  const int16_t lowLine = (lowerLimit_ > 0) ? lowerLimit_ : 50;
  const int16_t highLine = (upperLimit_ < 300) ? upperLimit_ : 300;
  for (int16_t d = lowLine; d < highLine; d += 50) {
    const int16_t yMin = ValueToVerticalPixel(d) - 1;
    const int16_t yMax = ValueToVerticalPixel(d + 50) + 1;
    const int16_t yRange = yMin - yMax;
    const size_t xSeed = ESP.random() % xRange;
    const size_t ySeed = ESP.random() % yRange;
    const size_t nbPixels =
        densities[currentDensity] * (float)(xRange * yRange);
    printf("d=%d : xMin=%d, xMax=%d, yMin=%d yMax=%d, npPixel=%u\n", d, xMin,
           xMax, yMin, yMax, nbPixels);
    for (size_t p = 0; p < nbPixels; p++) {
      int16_t x =
          kGraphXstart + (int16_t)(gRandoms[(p + xSeed) % kMaxRandomNumbers] % xRange);
      int16_t y = yMax + (int16_t)(gRandoms[(p + ySeed) % kMaxRandomNumbers] % yRange);
      redCanvas_->writePixel(x, y, 1);
    }
    currentDensity++;
  }
}

void GraphSamples::Grid() {
  // Grid (we also "clear" the Red canvas)
  for (int16_t h = 0; h < 5; h++) {
    int16_t x = kGraphXstart + h * kGraphWidth / 4;
    blackCanvas_->drawFastVLine(x, kGraphYstart, -kGraphHeight, 1);
    redCanvas_->drawFastVLine(x, kGraphYstart, -kGraphHeight, 0);
  }
  for (int16_t v = lowerLimit_; v <= upperLimit_; v += vStep_ / 2) {
    int16_t y = ValueToVerticalPixel(v);
    blackCanvas_->drawFastHLine(kGraphXstart, y, kGraphWidth, 1);
    redCanvas_->drawFastHLine(kGraphXstart, y, kGraphWidth, 0);
  }
}

void GraphSamples::Serie() {
  // Actually draw the Time Serie :-)
  int16_t y1 = 0;
  // It is actually necessary to draw twice: on the black buffer to see the curve,
  // and on the red buffer to clear any potential red pixel there.
  // This is due to the hardware turning red ink on always *after* the black ink.
  for (size_t i = 0; i < length_; i++) {
    int16_t data = Value(i);
    int16_t y2 = ValueToVerticalPixel(data);
    if (y1 != 0 && data != INT16_MIN) {
      if (y1 < y2) {
        blackCanvas_->drawFastVLine(i + kGraphXstart, y1 - 1, 2 + (y2 - y1), 1);
        redCanvas_->drawFastVLine(i + kGraphXstart, y1 - 1, 2 + (y2 - y1), 0);
      } else {
        blackCanvas_->drawFastVLine(i + kGraphXstart, y2 - 1, 2 + (y1 - y2), 1);
        redCanvas_->drawFastVLine(i + kGraphXstart, y2 - 1, 2 + (y1 - y2), 0);
      }
    }
    if (data == INT16_MIN) {
      y1 = 0;
    } else {
      y1 = y2;
    }
  }
}

void GraphSamples::Draw(GFXcanvas1 *black, GFXcanvas1 *red) {
  blackCanvas_ = black;
  redCanvas_ = red;
  // Round upper and lower limits to 100 AQI
  lowerLimit_ = min_ - min_ % vStep_;
  upperLimit_ = ((max_ - 1) / vStep_ + 1) * vStep_;

  Labels();
  Background();
  Grid();
  Serie();
}
