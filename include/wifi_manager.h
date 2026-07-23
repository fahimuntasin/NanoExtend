#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include "config.h"
#include "storage.h"

enum class WifiState : uint8_t {
  Idle = 0,
  ApOnly = 1,
  Scanning = 2,
  Connecting = 3,
  Connected = 4,
  Reconnecting = 5
};

struct ScanNetwork {
  char ssid[CFG_SSID_MAX + 1];
  int32_t rssi;
  uint8_t enc;
  uint8_t channel;
};

class WifiManager {
public:
  static void begin(const DeviceSettings& settings);
  static void loop();
  static WifiState state();
  static const char* stateName();
  static bool connect(const char* ssid, const char* pass, bool save);
  static void disconnectSta();
  static bool startScan(bool force);
  static bool scanReady();
  static size_t scanCount();
  static bool scanItem(size_t i, ScanNetwork& out);
  static void fillStatusJson(JsonObject obj);
  static void fillClientsJson(JsonArray arr);
  static void applyApSettings(const DeviceSettings& settings);
  static String staSsid();
  static int32_t rssi();
  static String bssid();
  static uint32_t connectedMs();
  static bool internetSharingActive();

private:
  static void startAp();
  static void tryAutoConnect();
  static void onConnected();
  static void onDisconnected();
  static void scheduleReconnect();
  static void processScan();

  static DeviceSettings settings_;
  static WifiState state_;
  static uint32_t connectStartedMs_;
  static uint32_t nextReconnectMs_;
  static uint32_t reconnectBackoffMs_;
  static uint32_t connectedSinceMs_;
  static uint32_t scanCacheMs_;
  static bool scanInProgress_;
  static bool scanCached_;
  static size_t scanCount_;
  static ScanNetwork scan_[CFG_SCAN_MAX];
  static uint8_t lastBssid_[6];
  static bool hasBssid_;
  static bool sharing_;
};
