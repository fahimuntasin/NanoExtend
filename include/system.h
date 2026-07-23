#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

enum class InternetState : uint8_t { Unknown = 0, Offline = 1, Degraded = 2, Online = 3 };

struct HealthResult {
  bool dnsOk;
  bool icmpOk;
  bool httpOk;
  bool httpsOk;
  InternetState state;
  char detail[96];
};

class SystemInfo {
public:
  static void begin();
  static void loop();
  static void setLastError(const char* err);
  static const char* lastError();
  static uint32_t uptimeSec();
  static uint32_t freeHeap();
  static uint32_t minFreeHeap();
  static uint32_t maxAllocHeap();
  static float fragmentationRatio();
  static uint32_t cpuFreqMhz();
  static const char* sdkVersion();
  static const char* resetReasonText();
  static uint32_t resetReasonCode();
  static uint32_t reconnectCount();
  static void bumpReconnect();
  static uint32_t bootCount();
  static InternetState internetState();
  static HealthResult lastHealth();
  static void requestHealthCheck();
  static void fillStatusJson(JsonObject obj);
  static void markStableBootIfReady();
  static bool previousCrashPending();
  static const char* previousCrashText();

private:
  static void runHealthCheck();
  static bool probeDns();
  static bool probeHttp();
  static bool probeHttps();
  static bool probeIcmp();

  static uint32_t bootMs_;
  static uint32_t reconnectCount_;
  static uint32_t bootCount_;
  static uint32_t resetReason_;
  static char lastError_[96];
  static char crashText_[96];
  static bool crashPending_;
  static bool healthRequested_;
  static uint32_t lastHealthMs_;
  static HealthResult health_;
  static bool stableCleared_;
};
