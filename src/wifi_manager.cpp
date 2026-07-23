#include "wifi_manager.h"

#include <esp_wifi.h>

#include "captive_portal.h"
#include "dns_manager.h"
#include "logger.h"
#include "nat_router.h"
#include "system.h"

DeviceSettings WifiManager::settings_ = {};
WifiState WifiManager::state_ = WifiState::Idle;
uint32_t WifiManager::connectStartedMs_ = 0;
uint32_t WifiManager::nextReconnectMs_ = 0;
uint32_t WifiManager::reconnectBackoffMs_ = CFG_RECONNECT_MIN_MS;
uint32_t WifiManager::connectedSinceMs_ = 0;
uint32_t WifiManager::scanCacheMs_ = 0;
bool WifiManager::scanInProgress_ = false;
bool WifiManager::scanCached_ = false;
size_t WifiManager::scanCount_ = 0;
ScanNetwork WifiManager::scan_[CFG_SCAN_MAX] = {};
uint8_t WifiManager::lastBssid_[6] = {};
bool WifiManager::hasBssid_ = false;
bool WifiManager::sharing_ = false;

namespace {
const char* encName(wifi_auth_mode_t m) {
  switch (m) {
  case WIFI_AUTH_OPEN:
    return "open";
  case WIFI_AUTH_WEP:
    return "wep";
  case WIFI_AUTH_WPA_PSK:
    return "wpa";
  case WIFI_AUTH_WPA2_PSK:
    return "wpa2";
  case WIFI_AUTH_WPA_WPA2_PSK:
    return "wpa/wpa2";
  case WIFI_AUTH_WPA2_ENTERPRISE:
    return "wpa2-e";
  case WIFI_AUTH_WPA3_PSK:
    return "wpa3";
  default:
    return "other";
  }
}

int encToByte(wifi_auth_mode_t m) {
  return static_cast<int>(m);
}
} // namespace

void WifiManager::begin(const DeviceSettings& settings) {
  settings_ = settings;
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  startAp();
  DnsManager::begin();
  CaptivePortal::begin(CFG_AP_IP);
  NatRouter::begin();
  state_ = WifiState::ApOnly;
  LOG_I("WiFi", "AP+STA mode ready");
  tryAutoConnect();
}

void WifiManager::startAp() {
  IPAddress ip = CFG_AP_IP;
  IPAddress gw = CFG_AP_GW;
  IPAddress mask = CFG_AP_MASK;
  WiFi.softAPConfig(ip, gw, mask);
  bool ok = WiFi.softAP(settings_.apSsid, settings_.apPass, CFG_AP_CHANNEL, CFG_AP_HIDDEN,
                        CFG_AP_MAX_CLIENTS);
  if (!ok) {
    LOG_E("WiFi", "SoftAP start failed");
    SystemInfo::setLastError("SoftAP start failed");
  } else {
    LOG_I("WiFi", "Starting AP... SSID=%s IP=%s max=%d", settings_.apSsid,
          WiFi.softAPIP().toString().c_str(), CFG_AP_MAX_CLIENTS);
  }
  if (settings_.hostname[0]) {
    WiFi.setHostname(settings_.hostname);
  }
}

void WifiManager::applyApSettings(const DeviceSettings& settings) {
  settings_ = settings;
  WiFi.softAPdisconnect(false);
  startAp();
}

void WifiManager::tryAutoConnect() {
  if (!settings_.autoReconnect || !settings_.hasStaCreds)
    return;
  LOG_I("WiFi", "Auto reconnect with saved credentials");
  connect(settings_.staSsid, settings_.staPass, false);
}

bool WifiManager::connect(const char* ssid, const char* pass, bool save) {
  if (!ssid || ssid[0] == '\0')
    return false;
  size_t passLen = pass ? strlen(pass) : 0;
  if (passLen < CFG_PASS_MIN || passLen > CFG_PASS_MAX) {
    SystemInfo::setLastError("Invalid password length");
    return false;
  }
  if (strlen(ssid) > CFG_SSID_MAX) {
    SystemInfo::setLastError("Invalid SSID length");
    return false;
  }

  if (NatRouter::isEnabled())
    NatRouter::disable();
  sharing_ = false;
  CaptivePortal::setActive(true);

  state_ = WifiState::Connecting;
  connectStartedMs_ = millis();
  LOG_I("WiFi", "Connecting... SSID=%s", ssid);
  WiFi.disconnect(false, false);
  WiFi.begin(ssid, pass);

  if (save) {
    Storage::saveStaCreds(ssid, pass);
    settings_ = Storage::load();
  }
  return true;
}

void WifiManager::disconnectSta() {
  if (NatRouter::isEnabled())
    NatRouter::disable();
  sharing_ = false;
  WiFi.disconnect(false, false);
  state_ = WifiState::ApOnly;
  DnsManager::onStaLost();
  CaptivePortal::setActive(true);
  hasBssid_ = false;
  LOG_I("WiFi", "STA disconnected by user");
}

void WifiManager::onConnected() {
  state_ = WifiState::Connected;
  connectedSinceMs_ = millis();
  reconnectBackoffMs_ = CFG_RECONNECT_MIN_MS;
  DnsManager::onStaGotIp();

  uint8_t* b = WiFi.BSSID();
  if (b) {
    memcpy(lastBssid_, b, 6);
    hasBssid_ = true;
  }

  bool natOk = NatRouter::enable();
  sharing_ = natOk;
  CaptivePortal::setActive(!natOk);
  SystemInfo::requestHealthCheck();
  LOG_I("WiFi", "Connected IP=%s GW=%s RSSI=%d NAT=%d", WiFi.localIP().toString().c_str(),
        WiFi.gatewayIP().toString().c_str(), WiFi.RSSI(), natOk ? 1 : 0);
  if (!natOk)
    SystemInfo::setLastError("NAT enable failed");
}

void WifiManager::onDisconnected() {
  if (state_ == WifiState::Connected || state_ == WifiState::Connecting ||
      state_ == WifiState::Reconnecting) {
    if (NatRouter::isEnabled())
      NatRouter::disable();
    sharing_ = false;
    DnsManager::onStaLost();
    CaptivePortal::setActive(true);
    SystemInfo::bumpReconnect();
    scheduleReconnect();
    state_ = WifiState::Reconnecting;
    LOG_W("WiFi", "STA lost; reconnect scheduled");
  }
}

void WifiManager::scheduleReconnect() {
  nextReconnectMs_ = millis() + reconnectBackoffMs_;
  reconnectBackoffMs_ = min(reconnectBackoffMs_ * 2, CFG_RECONNECT_MAX_MS);
  // jitter
  nextReconnectMs_ += (esp_random() % 250);
}

bool WifiManager::startScan(bool force) {
  if (scanInProgress_)
    return false;
  if (!force && scanCached_ && (millis() - scanCacheMs_ < CFG_SCAN_CACHE_MS)) {
    return true;
  }
  scanInProgress_ = true;
  state_ = (state_ == WifiState::Connected) ? state_ : WifiState::Scanning;
  // Async scan: non-blocking
  int16_t rc = WiFi.scanNetworks(true /*async*/, true /*show_hidden*/);
  if (rc == WIFI_SCAN_FAILED) {
    scanInProgress_ = false;
    LOG_E("WiFi", "Scan failed to start");
    return false;
  }
  LOG_I("WiFi", "Async scan started");
  return true;
}

void WifiManager::processScan() {
  if (!scanInProgress_)
    return;
  int16_t n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING)
    return;
  scanInProgress_ = false;
  if (n < 0) {
    LOG_E("WiFi", "Scan complete error %d", n);
    WiFi.scanDelete();
    return;
  }

  // Collect and sort by RSSI desc
  struct Tmp {
    int idx;
    int32_t rssi;
  } tmp[CFG_SCAN_MAX];
  size_t count = min(static_cast<size_t>(n), static_cast<size_t>(CFG_SCAN_MAX));
  for (size_t i = 0; i < count; i++) {
    tmp[i].idx = static_cast<int>(i);
    tmp[i].rssi = WiFi.RSSI(i);
  }
  for (size_t i = 0; i < count; i++) {
    for (size_t j = i + 1; j < count; j++) {
      if (tmp[j].rssi > tmp[i].rssi) {
        Tmp t = tmp[i];
        tmp[i] = tmp[j];
        tmp[j] = t;
      }
    }
  }

  scanCount_ = count;
  for (size_t i = 0; i < count; i++) {
    int idx = tmp[i].idx;
    ScanNetwork& sn = scan_[i];
    String ssid = WiFi.SSID(idx);
    strncpy(sn.ssid, ssid.c_str(), sizeof(sn.ssid) - 1);
    sn.ssid[sizeof(sn.ssid) - 1] = '\0';
    sn.rssi = WiFi.RSSI(idx);
    sn.enc = static_cast<uint8_t>(WiFi.encryptionType(idx));
    sn.channel = WiFi.channel(idx);
  }
  WiFi.scanDelete();
  scanCached_ = true;
  scanCacheMs_ = millis();
  if (state_ == WifiState::Scanning)
    state_ = WifiState::ApOnly;
  LOG_I("WiFi", "Scan cached %u networks", static_cast<unsigned>(scanCount_));
}

bool WifiManager::scanReady() {
  return scanCached_ && !scanInProgress_;
}
size_t WifiManager::scanCount() {
  return scanCount_;
}

bool WifiManager::scanItem(size_t i, ScanNetwork& out) {
  if (i >= scanCount_)
    return false;
  out = scan_[i];
  return true;
}

void WifiManager::loop() {
  processScan();
  CaptivePortal::loop();

  wl_status_t st = WiFi.status();
  if (state_ == WifiState::Connecting) {
    if (st == WL_CONNECTED) {
      onConnected();
    } else if (millis() - connectStartedMs_ > CFG_CONNECT_TIMEOUT_MS) {
      LOG_E("WiFi", "Connect timeout");
      SystemInfo::setLastError("Connect timeout");
      WiFi.disconnect(false, false);
      state_ = WifiState::ApOnly;
      scheduleReconnect();
      state_ = settings_.autoReconnect && settings_.hasStaCreds ? WifiState::Reconnecting
                                                                : WifiState::ApOnly;
    }
  } else if (state_ == WifiState::Connected) {
    if (st != WL_CONNECTED) {
      onDisconnected();
    } else if (hasBssid_) {
      uint8_t* b = WiFi.BSSID();
      if (b && memcmp(b, lastBssid_, 6) != 0) {
        LOG_W("WiFi", "Upstream BSSID changed; restoring NAT/DNS");
        memcpy(lastBssid_, b, 6);
        DnsManager::onStaGotIp();
        NatRouter::disable();
        bool ok = NatRouter::enable();
        sharing_ = ok;
        CaptivePortal::setActive(!ok);
        SystemInfo::requestHealthCheck();
        SystemInfo::bumpReconnect();
      }
    }
  } else if (state_ == WifiState::Reconnecting) {
    if (st == WL_CONNECTED) {
      onConnected();
    } else if (millis() >= nextReconnectMs_ && settings_.hasStaCreds && settings_.autoReconnect) {
      LOG_I("WiFi", "Reconnecting...");
      connect(settings_.staSsid, settings_.staPass, false);
    }
  }
}

WifiState WifiManager::state() {
  return state_;
}

const char* WifiManager::stateName() {
  switch (state_) {
  case WifiState::Idle:
    return "idle";
  case WifiState::ApOnly:
    return "ap_only";
  case WifiState::Scanning:
    return "scanning";
  case WifiState::Connecting:
    return "connecting";
  case WifiState::Connected:
    return "connected";
  case WifiState::Reconnecting:
    return "reconnecting";
  }
  return "unknown";
}

String WifiManager::staSsid() {
  return WiFi.SSID();
}
int32_t WifiManager::rssi() {
  return WiFi.RSSI();
}
String WifiManager::bssid() {
  return WiFi.BSSIDstr();
}
uint32_t WifiManager::connectedMs() {
  if (state_ != WifiState::Connected)
    return 0;
  return millis() - connectedSinceMs_;
}
bool WifiManager::internetSharingActive() {
  return sharing_ && NatRouter::isEnabled();
}

void WifiManager::fillStatusJson(JsonObject obj) {
  obj["state"] = stateName();
  obj["apSsid"] = settings_.apSsid;
  obj["apIp"] = WiFi.softAPIP().toString();
  obj["apClients"] = WiFi.softAPgetStationNum();
  obj["staConnected"] = WiFi.status() == WL_CONNECTED;
  obj["staSsid"] = WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "";
  obj["rssi"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  obj["bssid"] = WiFi.status() == WL_CONNECTED ? WiFi.BSSIDstr() : "";
  obj["staIp"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";
  obj["gateway"] = WiFi.status() == WL_CONNECTED ? WiFi.gatewayIP().toString() : "";
  obj["dns"] = WiFi.status() == WL_CONNECTED ? WiFi.dnsIP().toString() : "";
  obj["mac"] = WiFi.macAddress();
  obj["nat"] = NatRouter::isEnabled();
  obj["sharing"] = internetSharingActive();
  obj["connectedMs"] = connectedMs();
  obj["scanInProgress"] = scanInProgress_;
  obj["scanCached"] = scanCached_;
  obj["scanAgeMs"] = scanCached_ ? (millis() - scanCacheMs_) : 0;
}

void WifiManager::fillClientsJson(JsonArray arr) {
  wifi_sta_list_t sta;
  if (esp_wifi_ap_get_sta_list(&sta) != ESP_OK)
    return;
  for (int i = 0; i < sta.num && i < CFG_AP_MAX_CLIENTS + 2; i++) {
    JsonObject o = arr.add<JsonObject>();
    char mac[18];
    snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X", sta.sta[i].mac[0],
             sta.sta[i].mac[1], sta.sta[i].mac[2], sta.sta[i].mac[3], sta.sta[i].mac[4],
             sta.sta[i].mac[5]);
    o["mac"] = mac;
    o["rssi"] = 0;
    o["ip"] = "";
    o["hostname"] = "";
  }
}
