#include "storage.h"

#include <Preferences.h>
#include <string.h>

#include "logger.h"

namespace {
Preferences prefs;
constexpr uint32_t kCrashMagic = 0xC5A5C0DE;

uint32_t crc32_update(uint32_t crc, const uint8_t* data, size_t len) {
  crc = ~crc;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int b = 0; b < 8; b++) {
      uint32_t mask = -(crc & 1u);
      crc = (crc >> 1) ^ (0xEDB88320u & mask);
    }
  }
  return ~crc;
}
} // namespace

void Storage::defaults(DeviceSettings& s) {
  memset(&s, 0, sizeof(s));
  strncpy(s.apSsid, CFG_AP_SSID_DEFAULT, sizeof(s.apSsid) - 1);
  strncpy(s.apPass, CFG_AP_PASS_DEFAULT, sizeof(s.apPass) - 1);
  strncpy(s.deviceName, NANOEXTEND_NAME, sizeof(s.deviceName) - 1);
  strncpy(s.hostname, "nanoextend", sizeof(s.hostname) - 1);
  s.autoReconnect = true;
  s.hasStaCreds = false;
  s.schema = CFG_STORAGE_SCHEMA;
  s.generation = 1;
  s.checksum = 0;
}

bool Storage::valid(const DeviceSettings& s) {
  if (s.schema != CFG_STORAGE_SCHEMA)
    return false;
  if (s.apSsid[0] == '\0' || strlen(s.apSsid) > CFG_SSID_MAX)
    return false;
  size_t apLen = strlen(s.apPass);
  if (apLen < CFG_PASS_MIN || apLen > CFG_PASS_MAX)
    return false;
  if (s.hasStaCreds) {
    if (s.staSsid[0] == '\0' || strlen(s.staSsid) > CFG_SSID_MAX)
      return false;
    size_t staLen = strlen(s.staPass);
    if (staLen < CFG_PASS_MIN || staLen > CFG_PASS_MAX)
      return false;
  }
  DeviceSettings tmp = s;
  uint32_t stored = tmp.checksum;
  tmp.checksum = 0;
  return checksum(tmp) == stored;
}

uint32_t Storage::checksum(const DeviceSettings& s) {
  return crc32_update(0, reinterpret_cast<const uint8_t*>(&s), sizeof(s));
}

bool Storage::begin() {
  if (!prefs.begin(CFG_NVS_NAMESPACE, false)) {
    LOG_E("Storage", "Failed to open NVS namespace");
    return false;
  }
  LOG_I("Storage", "NVS ready schema=%u", CFG_STORAGE_SCHEMA);
  return true;
}

DeviceSettings Storage::load() {
  DeviceSettings s;
  defaults(s);

  size_t len = prefs.getBytesLength("cfg");
  if (len == sizeof(DeviceSettings)) {
    DeviceSettings loaded;
    prefs.getBytes("cfg", &loaded, sizeof(loaded));
    if (valid(loaded)) {
      LOG_I("Storage", "Loaded settings gen=%lu hasSta=%d",
            static_cast<unsigned long>(loaded.generation), loaded.hasStaCreds ? 1 : 0);
      return loaded;
    }
    LOG_W("Storage", "Invalid settings checksum/schema; using defaults");
  } else if (len > 0) {
    LOG_W("Storage", "Settings size mismatch (%u); using defaults", static_cast<unsigned>(len));
  } else {
    LOG_I("Storage", "No saved settings; using defaults");
  }

  // Persist clean defaults so power-loss mid-write can recover to a known good
  // blob.
  save(s);
  return s;
}

bool Storage::save(const DeviceSettings& settings) {
  DeviceSettings s = settings;
  if (!valid(s) && s.checksum == 0) {
    // Fill checksum for new records after field validation without checksum.
    DeviceSettings check = s;
    check.checksum = 0;
    if (check.apSsid[0] == '\0')
      return false;
    s.checksum = checksum(check);
  } else {
    DeviceSettings check = s;
    check.checksum = 0;
    s.checksum = checksum(check);
  }

  if (s.apSsid[0] == '\0' || strlen(s.apPass) < CFG_PASS_MIN) {
    LOG_E("Storage", "Refusing to save invalid AP settings");
    return false;
  }

  s.generation += 1;
  DeviceSettings check = s;
  check.checksum = 0;
  s.checksum = checksum(check);

  // Transaction-like: write payload, then generation marker.
  size_t written = prefs.putBytes("cfg", &s, sizeof(s));
  prefs.putUInt("cfg_gen", s.generation);
  bool ok = written == sizeof(s);
  if (ok) {
    LOG_I("Storage", "Saved settings gen=%lu", static_cast<unsigned long>(s.generation));
  } else {
    LOG_E("Storage", "Failed to write settings");
  }
  return ok;
}

bool Storage::saveStaCreds(const char* ssid, const char* pass) {
  if (!ssid || !pass)
    return false;
  size_t ssidLen = strlen(ssid);
  size_t passLen = strlen(pass);
  if (ssidLen == 0 || ssidLen > CFG_SSID_MAX)
    return false;
  if (passLen < CFG_PASS_MIN || passLen > CFG_PASS_MAX)
    return false;

  DeviceSettings s = load();
  strncpy(s.staSsid, ssid, sizeof(s.staSsid) - 1);
  s.staSsid[sizeof(s.staSsid) - 1] = '\0';
  strncpy(s.staPass, pass, sizeof(s.staPass) - 1);
  s.staPass[sizeof(s.staPass) - 1] = '\0';
  s.hasStaCreds = true;
  s.autoReconnect = true;
  s.checksum = 0;
  return save(s);
}

bool Storage::clearStaCreds() {
  DeviceSettings s = load();
  s.staSsid[0] = '\0';
  s.staPass[0] = '\0';
  s.hasStaCreds = false;
  s.checksum = 0;
  return save(s);
}

bool Storage::factoryReset() {
  prefs.clear();
  DeviceSettings s;
  defaults(s);
  bool ok = save(s);
  CrashRecord c = {};
  c.magic = kCrashMagic;
  saveCrash(c);
  LOG_W("Storage", "Factory reset complete");
  return ok;
}

CrashRecord Storage::loadCrash() {
  CrashRecord c = {};
  size_t len = prefs.getBytesLength("crash");
  if (len == sizeof(CrashRecord)) {
    prefs.getBytes("crash", &c, sizeof(c));
    if (c.magic != kCrashMagic) {
      memset(&c, 0, sizeof(c));
      c.magic = kCrashMagic;
    }
  } else {
    c.magic = kCrashMagic;
  }
  return c;
}

bool Storage::saveCrash(const CrashRecord& rec) {
  CrashRecord c = rec;
  c.magic = kCrashMagic;
  return prefs.putBytes("crash", &c, sizeof(c)) == sizeof(c);
}

bool Storage::clearCrashPending() {
  CrashRecord c = loadCrash();
  c.pending = false;
  c.lastError[0] = '\0';
  return saveCrash(c);
}
