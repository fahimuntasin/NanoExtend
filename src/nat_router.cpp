#include "nat_router.h"

#include <WiFi.h>
#include <esp_netif.h>

#include "logger.h"

#if __has_include("lwip/lwip_napt.h")
#include "lwip/lwip_napt.h"
#define NANO_HAS_LWIP_NAPT_H 1
#else
#define NANO_HAS_LWIP_NAPT_H 0
#endif

bool NatRouter::enabled_ = false;
bool NatRouter::available_ = false;

namespace {
#if NANO_HAS_LWIP_NAPT_H
struct NaptContext {
  u32_t address;
  int enable;
};

esp_err_t setNaptInTcpipContext(void* arg) {
  auto* ctx = static_cast<NaptContext*>(arg);
  if (!ctx)
    return ESP_ERR_INVALID_ARG;
  ip_napt_enable(ctx->address, ctx->enable);
  return ESP_OK;
}

bool setNapt(const IPAddress& apIp, bool enable) {
  NaptContext ctx{
      static_cast<u32_t>(static_cast<uint32_t>(apIp)),
      enable ? 1 : 0,
  };
  return esp_netif_tcpip_exec(setNaptInTcpipContext, &ctx) == ESP_OK;
}
#endif
} // namespace

bool NatRouter::symbolsAvailable() {
#if NANO_HAS_LWIP_NAPT_H
  return true;
#else
  return false;
#endif
}

const char* NatRouter::apiName() {
#if NANO_HAS_LWIP_NAPT_H
  return "ip_napt_enable(u32_t,int)";
#else
  return "unavailable";
#endif
}

bool NatRouter::begin() {
  available_ = symbolsAvailable();
  if (!available_) {
    LOG_E("NAT", "lwIP NAPT headers/symbols unavailable on this core");
    return false;
  }
  LOG_I("NAT", "Compatibility layer ready api=%s", apiName());
  return true;
}

bool NatRouter::enable() {
  if (!available_ && !begin())
    return false;
  if (WiFi.status() != WL_CONNECTED) {
    LOG_W("NAT", "Refuse enable: STA not connected");
    return false;
  }

  IPAddress apIp = WiFi.softAPIP();
  if (apIp == IPAddress(0, 0, 0, 0)) {
    LOG_E("NAT", "SoftAP IP invalid");
    return false;
  }

#if NANO_HAS_LWIP_NAPT_H
  // ip_napt_enable() starts lwIP timers. IDF 5.x requires it to execute
  // inside the TCP/IP thread or it asserts in sys_timeout().
  if (!setNapt(apIp, true)) {
    LOG_E("NAT", "TCP/IP context enable failed");
    enabled_ = false;
    return false;
  }
  enabled_ = true;
  LOG_I("NAT", "Enabled on %s", apIp.toString().c_str());
  return true;
#else
  LOG_E("NAT", "Cannot enable: symbols missing");
  enabled_ = false;
  return false;
#endif
}

bool NatRouter::disable() {
#if NANO_HAS_LWIP_NAPT_H
  IPAddress apIp = WiFi.softAPIP();
  if (apIp != IPAddress(0, 0, 0, 0)) {
    if (!setNapt(apIp, false)) {
      LOG_W("NAT", "TCP/IP context disable failed");
    }
  }
#endif
  enabled_ = false;
  LOG_I("NAT", "Disabled");
  return true;
}

bool NatRouter::isEnabled() {
  return enabled_;
}
