#pragma once

#include <Arduino.h>
#include <IPAddress.h>

class NatRouter {
public:
  static bool begin();
  static bool enable();
  static bool disable();
  static bool isEnabled();
  static const char* apiName();
  static bool symbolsAvailable();

private:
  static bool enabled_;
  static bool available_;
};
