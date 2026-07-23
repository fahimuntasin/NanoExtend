#pragma once

#include <Arduino.h>
#include <DNSServer.h>

class CaptivePortal {
public:
  static void begin(const IPAddress& apIp);
  static void loop();
  static void setActive(bool active);
  static bool isActive();
  static bool handleCaptiveRequest(const String& host, const String& path);

private:
  static DNSServer dns_;
  static bool active_;
  static IPAddress apIp_;
};
