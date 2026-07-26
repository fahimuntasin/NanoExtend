#pragma once

#include <Arduino.h>

// Onboard LED celebrate patterns for USB dashboard connect feedback.
class StatusLed {
public:
  static void begin();
  static void loop();
  // Double-pulse pair — feels like a cheerful “hello” on DevKit blue LED (GPIO2).
  static void celebrateConnect();
  static void blinkError();

private:
  static void writeLed(bool on);
  static uint8_t pin_;
  static bool activeLow_;
  static uint8_t step_;
  static uint32_t nextMs_;
  static uint8_t pattern_;
};
