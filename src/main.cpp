#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "logger.h"
#include "ota.h"
#include "serial_admin.h"
#include "storage.h"
#include "system.h"
#include "version.h"
#include "web_server.h"
#include "wifi_manager.h"

void setup() {
  Logger::begin(115200);
  LOG_I("System", "NanoExtend %s starting", NANOEXTEND_FW_VERSION);
  LOG_I("System", "Platform pin %s target core %s", NANOEXTEND_PLATFORM_PIN,
        NANOEXTEND_ARDUINO_CORE_TARGET);

  if (!Storage::begin()) {
    LOG_E("System", "Storage init failed");
  }

  SystemInfo::begin();
  OtaManager::begin();

  DeviceSettings settings = Storage::load();
  WifiManager::begin(settings);
  WebServerApp::begin();
  SerialAdmin::begin();

  LOG_I("System", "Ready AP=%s IP=%s", settings.apSsid, WiFi.softAPIP().toString().c_str());
}

void loop() {
  WifiManager::loop();
  SystemInfo::loop();
  WebServerApp::loop();
  SerialAdmin::loop();
  // Cooperative yield only — never long delay().
  delay(1);
}
