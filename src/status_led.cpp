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
  // Most ESP32 DevKit blue LEDs are active-low on GPIO2.
  digitalWrite(pin_, (on ^ activeLow_) ? HIGH : LOW);
}

void StatusLed::begin() {
#if defined(LED_BUILTIN)
  pin_ = LED_BUILTIN;
#else
  pin_ = 2;
#endif
  pinMode(pin_, OUTPUT);
  writeLed(false);
  pattern_ = kPatternIdle;
  step_ = 0;
  LOG_I("LED", "Status LED on GPIO%u", static_cast<unsigned>(pin_));
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
    // on 70, off 70, on 70, off 220, on 70, off 70, on 70, off — done
    static const uint16_t dur[] = {70, 70, 70, 220, 70, 70, 70, 400};
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
    static const uint16_t dur[] = {120, 120, 120, 120, 120, 400};
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
