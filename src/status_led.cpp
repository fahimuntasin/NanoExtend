#include "status_led.h"

#include "logger.h"

namespace {
constexpr uint8_t kPatternIdle = 0;
constexpr uint8_t kPatternCelebrate = 1;
constexpr uint8_t kPatternError = 2;
} // namespace

uint8_t StatusLed::pin_ = 2;
bool StatusLed::activeLow_ = true;
uint8_t StatusLed::step_ = 0;
uint32_t StatusLed::nextMs_ = 0;
uint8_t StatusLed::pattern_ = kPatternIdle;

void StatusLed::writeLed(bool on) {
  // DevKit blue LED is typically active-low on GPIO2.
  digitalWrite(pin_, (on ^ activeLow_) ? HIGH : LOW);
}

void StatusLed::begin() {
  // Prefer GPIO2 on classic ESP32 DevKit (blue LED), even if LED_BUILTIN differs.
  pin_ = 2;
  pinMode(pin_, OUTPUT);
  writeLed(false);
  pattern_ = kPatternIdle;
  step_ = 0;
  LOG_I("LED", "Status LED on GPIO%u activeLow=%d", static_cast<unsigned>(pin_),
        activeLow_ ? 1 : 0);
}

void StatusLed::celebrateConnect() {
  pattern_ = kPatternCelebrate;
  step_ = 0;
  nextMs_ = millis();
}

void StatusLed::blinkError() {
  pattern_ = kPatternError;
  step_ = 0;
  nextMs_ = millis();
}

void StatusLed::loop() {
  if (pattern_ == kPatternIdle)
    return;
  const uint32_t now = millis();
  if (now < nextMs_)
    return;

  if (pattern_ == kPatternCelebrate) {
    // Longer pulses so the DevKit blue LED is obvious: ♪♪ … ♪♪
    static const uint16_t dur[] = {180, 100, 180, 280, 180, 100, 180, 500};
    static const bool on[] = {true, false, true, false, true, false, true, false};
    if (step_ >= sizeof(dur) / sizeof(dur[0])) {
      writeLed(false);
      pattern_ = kPatternIdle;
      step_ = 0;
      return;
    }
    writeLed(on[step_]);
    nextMs_ = now + dur[step_];
    step_++;
    return;
  }

  if (pattern_ == kPatternError) {
    static const uint16_t dur[] = {250, 150, 250, 150, 250, 500};
    static const bool on[] = {true, false, true, false, true, false};
    if (step_ >= sizeof(dur) / sizeof(dur[0])) {
      writeLed(false);
      pattern_ = kPatternIdle;
      step_ = 0;
      return;
    }
    writeLed(on[step_]);
    nextMs_ = now + dur[step_];
    step_++;
  }
}
