#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

class OtaManager {
public:
  static void begin();
  static bool isBusy();
  static int progress();
  static const char* status();
  static void setExpectedSha256(const char* sha256);
  static bool beginUpdate(size_t contentLength);
  static bool writeChunk(const uint8_t* data, size_t len);
  static bool endUpdate(bool success);
  static void abortUpdate(const char* reason);
  static bool confirmRunningImage();
  static bool pendingVerification();
  static const char* computedSha256();
  static void fillJson(JsonObject obj);

private:
  static bool busy_;
  static int progress_;
  static size_t written_;
  static size_t total_;
  static char status_[64];
  static char expectedSha256_[65];
  static char computedSha256_[65];
  static bool pendingVerification_;
};
