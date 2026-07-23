#include "system.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_system.h>
#include <lwip/dns.h>
#include <lwip/inet.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>

#include "config.h"
#include "logger.h"
#include "nat_router.h"
#include "ota.h"
#include "storage.h"
#include "version.h"

uint32_t SystemInfo::bootMs_ = 0;
uint32_t SystemInfo::reconnectCount_ = 0;
uint32_t SystemInfo::bootCount_ = 0;
uint32_t SystemInfo::resetReason_ = 0;
char SystemInfo::lastError_[96] = {0};
char SystemInfo::crashText_[96] = {0};
bool SystemInfo::crashPending_ = false;
bool SystemInfo::healthRequested_ = false;
uint32_t SystemInfo::lastHealthMs_ = 0;
HealthResult SystemInfo::health_ = {};
bool SystemInfo::stableCleared_ = false;

namespace {
const char* resetToText(esp_reset_reason_t r) {
  switch (r) {
  case ESP_RST_POWERON:
    return "POWERON";
  case ESP_RST_EXT:
    return "EXT";
  case ESP_RST_SW:
    return "SW";
  case ESP_RST_PANIC:
    return "PANIC";
  case ESP_RST_INT_WDT:
    return "INT_WDT";
  case ESP_RST_TASK_WDT:
    return "TASK_WDT";
  case ESP_RST_WDT:
    return "WDT";
  case ESP_RST_DEEPSLEEP:
    return "DEEPSLEEP";
  case ESP_RST_BROWNOUT:
    return "BROWNOUT";
  case ESP_RST_SDIO:
    return "SDIO";
  default:
    return "UNKNOWN";
  }
}
} // namespace

void SystemInfo::begin() {
  bootMs_ = millis();
  resetReason_ = static_cast<uint32_t>(esp_reset_reason());
  CrashRecord c = Storage::loadCrash();
  bootCount_ = c.bootCount + 1;
  c.bootCount = bootCount_;
  c.resetReason = resetReason_;
  if (resetReason_ == ESP_RST_PANIC || resetReason_ == ESP_RST_INT_WDT ||
      resetReason_ == ESP_RST_TASK_WDT || resetReason_ == ESP_RST_WDT ||
      resetReason_ == ESP_RST_BROWNOUT) {
    c.pending = true;
    c.lastCrashMs = millis();
    snprintf(c.lastError, sizeof(c.lastError), "reset=%s", resetToText(esp_reset_reason()));
  }
  Storage::saveCrash(c);
  crashPending_ = c.pending;
  strncpy(crashText_, c.lastError, sizeof(crashText_) - 1);
  health_.state = InternetState::Unknown;
  strncpy(health_.detail, "not checked", sizeof(health_.detail) - 1);
  LOG_I("System", "Boot #%lu reason=%s", static_cast<unsigned long>(bootCount_), resetReasonText());
}

void SystemInfo::loop() {
  markStableBootIfReady();
  if (healthRequested_ || (millis() - lastHealthMs_ > CFG_HEALTH_INTERVAL_MS)) {
    healthRequested_ = false;
    runHealthCheck();
  }
}

void SystemInfo::markStableBootIfReady() {
  if (stableCleared_)
    return;
  if (millis() - bootMs_ < CFG_STABLE_BOOT_MS)
    return;
  if (crashPending_) {
    Storage::clearCrashPending();
    crashPending_ = false;
    LOG_I("System", "Stable boot; cleared previous crash pending flag");
  }
  OtaManager::confirmRunningImage();
  stableCleared_ = true;
}

void SystemInfo::setLastError(const char* err) {
  strncpy(lastError_, err ? err : "", sizeof(lastError_) - 1);
  lastError_[sizeof(lastError_) - 1] = '\0';
  if (err && err[0])
    LOG_E("System", "%s", err);
}

const char* SystemInfo::lastError() {
  return lastError_;
}
uint32_t SystemInfo::uptimeSec() {
  return (millis() - bootMs_) / 1000UL;
}
uint32_t SystemInfo::freeHeap() {
  return ESP.getFreeHeap();
}
uint32_t SystemInfo::minFreeHeap() {
  return ESP.getMinFreeHeap();
}
uint32_t SystemInfo::maxAllocHeap() {
  return ESP.getMaxAllocHeap();
}

float SystemInfo::fragmentationRatio() {
  uint32_t freeH = freeHeap();
  uint32_t maxA = maxAllocHeap();
  if (freeH == 0)
    return 1.0f;
  return 1.0f - (static_cast<float>(maxA) / static_cast<float>(freeH));
}

uint32_t SystemInfo::cpuFreqMhz() {
  return getCpuFrequencyMhz();
}
const char* SystemInfo::sdkVersion() {
  return ESP.getSdkVersion();
}
uint32_t SystemInfo::resetReasonCode() {
  return resetReason_;
}
const char* SystemInfo::resetReasonText() {
  return resetToText(static_cast<esp_reset_reason_t>(resetReason_));
}
uint32_t SystemInfo::reconnectCount() {
  return reconnectCount_;
}
void SystemInfo::bumpReconnect() {
  reconnectCount_++;
}
uint32_t SystemInfo::bootCount() {
  return bootCount_;
}
InternetState SystemInfo::internetState() {
  return health_.state;
}
HealthResult SystemInfo::lastHealth() {
  return health_;
}
void SystemInfo::requestHealthCheck() {
  healthRequested_ = true;
}
bool SystemInfo::previousCrashPending() {
  return crashPending_;
}
const char* SystemInfo::previousCrashText() {
  return crashText_;
}

bool SystemInfo::probeDns() {
  IPAddress ip;
  return WiFi.hostByName(CFG_HEALTH_DNS_HOST, ip) == 1 && ip != IPAddress(0, 0, 0, 0);
}

bool SystemInfo::probeHttp() {
  HTTPClient http;
  http.setConnectTimeout(CFG_HEALTH_TIMEOUT_MS);
  http.setTimeout(CFG_HEALTH_TIMEOUT_MS);
  if (!http.begin(CFG_HEALTH_HTTP_URL))
    return false;
  int code = http.GET();
  http.end();
  return code > 0 && code < 500;
}

bool SystemInfo::probeHttps() {
  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(CFG_HEALTH_TIMEOUT_MS / 1000);
  if (!client.connect(CFG_HEALTH_HTTPS_HOST, CFG_HEALTH_HTTPS_PORT))
    return false;
  client.stop();
  return true;
}

bool SystemInfo::probeIcmp() {
  // ESP32 Arduino lacks portable non-root ICMP; approximate with TCP connect
  // to 1.1.1.1:53.
  WiFiClient c;
  c.setTimeout(CFG_HEALTH_TIMEOUT_MS / 1000);
  bool ok = c.connect(CFG_DNS_CLOUDFLARE, 53);
  c.stop();
  return ok;
}

void SystemInfo::runHealthCheck() {
  lastHealthMs_ = millis();
  if (WiFi.status() != WL_CONNECTED) {
    health_ = {};
    health_.state = InternetState::Offline;
    strncpy(health_.detail, "STA disconnected", sizeof(health_.detail) - 1);
    return;
  }

  health_.dnsOk = probeDns();
  health_.icmpOk = probeIcmp();
  health_.httpOk = probeHttp();
  health_.httpsOk = probeHttps();

  int score = (health_.dnsOk ? 1 : 0) + (health_.icmpOk ? 1 : 0) + (health_.httpOk ? 1 : 0) +
              (health_.httpsOk ? 1 : 0);

  if (score >= 3) {
    health_.state = InternetState::Online;
    snprintf(health_.detail, sizeof(health_.detail), "ok dns=%d icmp=%d http=%d https=%d",
             health_.dnsOk, health_.icmpOk, health_.httpOk, health_.httpsOk);
  } else if (score >= 1) {
    health_.state = InternetState::Degraded;
    snprintf(health_.detail, sizeof(health_.detail), "degraded dns=%d icmp=%d http=%d https=%d",
             health_.dnsOk, health_.icmpOk, health_.httpOk, health_.httpsOk);
    if (NatRouter::isEnabled() && score == 0) {
      NatRouter::disable();
    }
  } else {
    health_.state = InternetState::Offline;
    snprintf(health_.detail, sizeof(health_.detail), "fail dns=%d icmp=%d http=%d https=%d",
             health_.dnsOk, health_.icmpOk, health_.httpOk, health_.httpsOk);
    if (NatRouter::isEnabled()) {
      NatRouter::disable();
      setLastError("NAT validation failed");
    }
  }
  LOG_I("Health", "%s", health_.detail);
}

void SystemInfo::fillStatusJson(JsonObject obj) {
  obj["firmware"] = NANOEXTEND_FW_VERSION;
  obj["api"] = NANOEXTEND_API_VERSION;
  obj["uptimeSec"] = uptimeSec();
  obj["freeHeap"] = freeHeap();
  obj["minFreeHeap"] = minFreeHeap();
  obj["maxAllocHeap"] = maxAllocHeap();
  obj["fragmentation"] = fragmentationRatio();
  obj["cpuMhz"] = cpuFreqMhz();
  obj["sdk"] = sdkVersion();
  obj["resetReason"] = resetReasonText();
  obj["bootCount"] = bootCount_;
  obj["reconnectCount"] = reconnectCount_;
  obj["lastError"] = lastError_;
  obj["crashPending"] = crashPending_;
  obj["crashText"] = crashText_;
  obj["flashSize"] = ESP.getFlashChipSize();
  obj["sketchSize"] = ESP.getSketchSize();
  obj["freeSketch"] = ESP.getFreeSketchSpace();
  JsonObject h = obj["health"].to<JsonObject>();
  h["dns"] = health_.dnsOk;
  h["icmp"] = health_.icmpOk;
  h["http"] = health_.httpOk;
  h["https"] = health_.httpsOk;
  const char* st = "unknown";
  if (health_.state == InternetState::Online)
    st = "online";
  else if (health_.state == InternetState::Degraded)
    st = "degraded";
  else if (health_.state == InternetState::Offline)
    st = "offline";
  h["state"] = st;
  h["detail"] = health_.detail;
}
