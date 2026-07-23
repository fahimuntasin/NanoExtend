#pragma once

#include <Arduino.h>
#include <IPAddress.h>

class DnsManager {
public:
  static void begin();
  static void onStaGotIp();
  static void onStaLost();
  static IPAddress primaryDns();
  static IPAddress secondaryDns();
  static void applyToSoftApClients();
  static const char* sourceName();

private:
  static IPAddress primary_;
  static IPAddress secondary_;
  static const char* source_;
};
