#include "serial_admin.h"

#include <ArduinoJson.h>
#include <WiFi.h>
#include <cstring>

#include "config.h"
#include "logger.h"
#include "ota.h"
#include "status_led.h"
#include "storage.h"
#include "system.h"
#include "version.h"
#include "wifi_manager.h"

namespace {

constexpr size_t kLineMax = 768;
constexpr uint32_t kRateWindowMs = 1000;
constexpr uint8_t kRateMax = 24;

char lineBuf_[kLineMax];
size_t lineLen_ = 0;
uint32_t rateWindowStart_ = 0;
uint8_t rateCount_ = 0;

bool rateOk() {
  const uint32_t now = millis();
  if (now - rateWindowStart_ > kRateWindowMs) {
    rateWindowStart_ = now;
    rateCount_ = 0;
  }
  if (rateCount_ >= kRateMax)
    return false;
  rateCount_++;
  return true;
}

bool sanitizeSsid(const char* ssid) {
  if (!ssid)
    return false;
  const size_t n = strlen(ssid);
  if (n < 1 || n > CFG_SSID_MAX)
    return false;
  for (size_t i = 0; i < n; i++) {
    if (static_cast<uint8_t>(ssid[i]) < 0x20)
      return false;
  }
  return true;
}

bool sanitizePass(const char* pass) {
  if (!pass)
    return false;
  const size_t n = strlen(pass);
  return n >= CFG_PASS_MIN && n <= CFG_PASS_MAX;
}

bool sanitizeName(const char* name, size_t maxLen) {
  if (!name)
    return false;
  const size_t n = strlen(name);
  if (n < 1 || n > maxLen)
    return false;
  for (size_t i = 0; i < n; i++) {
    const uint8_t c = static_cast<uint8_t>(name[i]);
    if (c < 0x20 || c == '"' || c == '\\')
      return false;
  }
  return true;
}

void reply(uint32_t id, bool ok, JsonVariantConst result, const char* error) {
  JsonDocument doc;
  doc["v"] = 1;
  doc["id"] = id;
  doc["ok"] = ok;
  if (ok)
    doc["result"] = result;
  else
    doc["error"] = error ? error : "error";
  Serial.print("NE");
  serializeJson(doc, Serial);
  Serial.print('\n');
}

void replyOk(uint32_t id, JsonDocument& result) {
  reply(id, true, result.as<JsonVariantConst>(), nullptr);
}

void replyErr(uint32_t id, const char* error) {
  JsonDocument empty;
  reply(id, false, empty.as<JsonVariantConst>(), error);
}

void fillScanResult(JsonDocument& out) {
  out["cached"] = WifiManager::scanReady();
  JsonDocument st;
  JsonObject w = st.to<JsonObject>();
  WifiManager::fillStatusJson(w);
  out["ageMs"] = w["scanAgeMs"];
  out["inProgress"] = w["scanInProgress"];
  JsonArray arr = out["networks"].to<JsonArray>();
  const size_t n = WifiManager::scanCount();
  for (size_t i = 0; i < n; i++) {
    ScanNetwork sn;
    if (!WifiManager::scanItem(i, sn))
      continue;
    JsonObject o = arr.add<JsonObject>();
    o["ssid"] = sn.ssid;
    o["rssi"] = sn.rssi;
    o["channel"] = sn.channel;
    o["encryption"] = sn.enc;
  }
}

void handleCommand(JsonDocument& req) {
  const uint32_t id = req["id"] | 0U;
  const char* cmd = req["cmd"] | "";
  if (!cmd[0]) {
    replyErr(id, "missing cmd");
    return;
  }
  if (!rateOk()) {
    replyErr(id, "rate limited");
    return;
  }

  if (strcmp(cmd, "ping") == 0 || strcmp(cmd, "hello") == 0) {
    JsonDocument result;
    result["fw"] = NANOEXTEND_FW_VERSION;
    result["api"] = NANOEXTEND_API_VERSION;
    result["name"] = NANOEXTEND_NAME;
    result["channel"] = "usb-serial";
    result["dashboard"] = true;
    result["led"] = true;
    StatusLed::celebrateConnect();
    replyOk(id, result);
    return;
  }

  if (strcmp(cmd, "celebrate") == 0) {
    StatusLed::celebrateConnect();
    JsonDocument result;
    result["led"] = "celebrate";
    replyOk(id, result);
    return;
  }

  if (strcmp(cmd, "status") == 0) {
    JsonDocument result;
    JsonObject wifi = result["wifi"].to<JsonObject>();
    WifiManager::fillStatusJson(wifi);
    JsonObject sys = result["system"].to<JsonObject>();
    SystemInfo::fillStatusJson(sys);
    result["apIp"] = WiFi.softAPIP().toString();
    result["staIp"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";
    replyOk(id, result);
    return;
  }

  if (strcmp(cmd, "scan") == 0) {
    const bool refresh = req["refresh"] | true;
    if (OtaManager::isBusy()) {
      replyErr(id, "busy");
      return;
    }
    if (refresh)
      WifiManager::startScan(true);
    else if (!WifiManager::scanReady())
      WifiManager::startScan(false);
    JsonDocument result;
    fillScanResult(result);
    replyOk(id, result);
    return;
  }

  if (strcmp(cmd, "connect") == 0) {
    if (OtaManager::isBusy()) {
      replyErr(id, "busy");
      return;
    }
    const char* ssid = req["ssid"] | "";
    const char* pass = req["password"] | "";
    if (!sanitizeSsid(ssid) || !sanitizePass(pass)) {
      replyErr(id, "invalid ssid/password");
      return;
    }
    if (!WifiManager::connect(ssid, pass, true)) {
      replyErr(id, SystemInfo::lastError());
      return;
    }
    JsonDocument result;
    result["accepted"] = true;
    result["ssid"] = ssid;
    JsonObject wifi = result["wifi"].to<JsonObject>();
    WifiManager::fillStatusJson(wifi);
    replyOk(id, result);
    return;
  }

  if (strcmp(cmd, "disconnect") == 0) {
    WifiManager::disconnectSta();
    JsonDocument result;
    JsonObject wifi = result["wifi"].to<JsonObject>();
    WifiManager::fillStatusJson(wifi);
    replyOk(id, result);
    return;
  }

  if (strcmp(cmd, "settings_get") == 0) {
    DeviceSettings s = Storage::load();
    JsonDocument result;
    result["apSsid"] = s.apSsid;
    result["apPass"] = s.apPass;
    result["deviceName"] = s.deviceName;
    result["hostname"] = s.hostname;
    result["hasStaCreds"] = s.hasStaCreds;
    result["staSsid"] = s.hasStaCreds ? s.staSsid : "";
    result["autoReconnect"] = s.autoReconnect;
    replyOk(id, result);
    return;
  }

  if (strcmp(cmd, "settings_set") == 0) {
    const char* apSsid = req["apSsid"] | "";
    const char* apPass = req["apPass"] | "";
    const char* deviceName = req["deviceName"] | "";
    const char* hostname = req["hostname"] | "";
    if (!sanitizeSsid(apSsid) || !sanitizePass(apPass) ||
        !sanitizeName(deviceName, CFG_DEVICE_NAME_MAX) ||
        !sanitizeName(hostname, CFG_HOSTNAME_MAX)) {
      replyErr(id, "invalid settings");
      return;
    }
    DeviceSettings s = Storage::load();
    strlcpy(s.apSsid, apSsid, sizeof(s.apSsid));
    strlcpy(s.apPass, apPass, sizeof(s.apPass));
    strlcpy(s.deviceName, deviceName, sizeof(s.deviceName));
    strlcpy(s.hostname, hostname, sizeof(s.hostname));
    s.checksum = 0;
    if (!Storage::save(s)) {
      replyErr(id, "save failed");
      return;
    }
    WifiManager::applyApSettings(Storage::load());
    JsonDocument result;
    result["saved"] = true;
    replyOk(id, result);
    return;
  }

  if (strcmp(cmd, "reboot") == 0) {
    JsonDocument result;
    result["rebooting"] = true;
    replyOk(id, result);
    delay(80);
    ESP.restart();
    return;
  }

  if (strcmp(cmd, "clients") == 0) {
    JsonDocument result;
    JsonArray arr = result["clients"].to<JsonArray>();
    WifiManager::fillClientsJson(arr);
    result["count"] = arr.size();
    replyOk(id, result);
    return;
  }

  if (strcmp(cmd, "logs") == 0) {
    String text = Logger::exportText();
    constexpr size_t kMax = 1800;
    const bool truncated = text.length() > kMax;
    if (truncated)
      text = text.substring(text.length() - kMax);
    JsonDocument result;
    result["count"] = Logger::count();
    result["text"] = text;
    result["truncated"] = truncated;
    replyOk(id, result);
    return;
  }

  if (strcmp(cmd, "health") == 0) {
    SystemInfo::requestHealthCheck();
    JsonDocument result;
    JsonObject sys = result["system"].to<JsonObject>();
    SystemInfo::fillStatusJson(sys);
    replyOk(id, result);
    return;
  }

  if (strcmp(cmd, "factory_reset") == 0) {
    const bool confirm = req["confirm"] | false;
    if (!confirm) {
      replyErr(id, "confirm required");
      return;
    }
    Storage::factoryReset();
    JsonDocument result;
    result["reset"] = true;
    result["rebooting"] = true;
    replyOk(id, result);
    delay(80);
    ESP.restart();
    return;
  }

  replyErr(id, "unknown cmd");
}

void processLine(char* line) {
  // Strip CR
  size_t n = strlen(line);
  while (n > 0 && (line[n - 1] == '\r' || line[n - 1] == '\n')) {
    line[--n] = '\0';
  }
  if (n < 4)
    return;
  if (line[0] != 'N' || line[1] != 'E' || line[2] != '>')
    return;

  JsonDocument req;
  const DeserializationError err = deserializeJson(req, line + 3);
  if (err) {
    replyErr(0, "invalid json");
    return;
  }
  if ((req["v"] | 1) != 1) {
    replyErr(req["id"] | 0U, "unsupported protocol");
    return;
  }
  handleCommand(req);
}

} // namespace

void SerialAdmin::begin() {
  lineLen_ = 0;
  rateWindowStart_ = millis();
  rateCount_ = 0;
  LOG_I("SerialAdmin", "USB serial admin ready (prefix NE>)");
}

void SerialAdmin::loop() {
  while (Serial.available() > 0) {
    const int c = Serial.read();
    if (c < 0)
      break;
    if (c == '\n') {
      lineBuf_[lineLen_] = '\0';
      if (lineLen_ > 0)
        processLine(lineBuf_);
      lineLen_ = 0;
      continue;
    }
    if (c == '\r')
      continue;
    if (lineLen_ + 1 >= kLineMax) {
      lineLen_ = 0;
      replyErr(0, "line too long");
      continue;
    }
    lineBuf_[lineLen_++] = static_cast<char>(c);
  }
}
