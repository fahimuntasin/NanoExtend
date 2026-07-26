#pragma once

#include "config.h"
#include <Arduino.h>

struct DeviceSettings {
  char apSsid[CFG_SSID_MAX + 1];
  char apPass[CFG_PASS_MAX + 1];
  char deviceName[CFG_DEVICE_NAME_MAX + 1];
  char hostname[CFG_HOSTNAME_MAX + 1];
  char staSsid[CFG_SSID_MAX + 1];
  char staPass[CFG_PASS_MAX + 1];
  bool autoReconnect;
  bool hasStaCreds;
  uint32_t schema;
  uint32_t generation;
  uint32_t checksum;
};

struct CrashRecord {
  uint32_t magic;
  uint32_t resetReason;
  uint32_t bootCount;
  uint32_t lastCrashMs;
  char lastError[96];
  bool pending;
};

class Storage {
public:
  static bool begin();
  static DeviceSettings load();
  static bool save(const DeviceSettings& settings);
  static bool saveStaCreds(const char* ssid, const char* pass);
  static bool clearStaCreds();
  static bool factoryReset();
  static CrashRecord loadCrash();
  static bool saveCrash(const CrashRecord& rec);
  static bool clearCrashPending();
  static uint32_t checksum(const DeviceSettings& s);

private:
  static void defaults(DeviceSettings& s);
  static bool valid(const DeviceSettings& s);
};
