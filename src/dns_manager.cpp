#include "dns_manager.h"

#include <WiFi.h>
#include <esp_netif.h>
#include <string.h>

#include "config.h"
#include "logger.h"

namespace {
struct DnsApplyContext {
  esp_netif_t* netif;
  esp_netif_dns_info_t dns;
};

esp_err_t applyDnsInTcpipContext(void* arg) {
  auto* ctx = static_cast<DnsApplyContext*>(arg);
  if (!ctx || !ctx->netif)
    return ESP_ERR_INVALID_ARG;

  esp_netif_dhcp_status_t status = ESP_NETIF_DHCP_INIT;
  esp_err_t statusErr = esp_netif_dhcps_get_status(ctx->netif, &status);
  if (statusErr != ESP_OK)
    return statusErr;

  const bool wasStarted = status == ESP_NETIF_DHCP_STARTED;
  if (wasStarted) {
    esp_err_t stopErr = esp_netif_dhcps_stop(ctx->netif);
    if (stopErr != ESP_OK && stopErr != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) {
      return stopErr;
    }
  }

  esp_err_t result = esp_netif_set_dns_info(ctx->netif, ESP_NETIF_DNS_MAIN, &ctx->dns);
  if (result == ESP_OK) {
    uint8_t offerDns = 1;
    result = esp_netif_dhcps_option(ctx->netif, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER,
                                    &offerDns, sizeof(offerDns));
  }

  if (wasStarted) {
    esp_err_t startErr = esp_netif_dhcps_start(ctx->netif);
    if (result == ESP_OK && startErr != ESP_OK &&
        startErr != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED) {
      result = startErr;
    }
  }
  return result;
}
} // namespace

IPAddress DnsManager::primary_ = CFG_DNS_CLOUDFLARE;
IPAddress DnsManager::secondary_ = CFG_DNS_GOOGLE;
const char* DnsManager::source_ = "fallback";

void DnsManager::begin() {
  primary_ = CFG_DNS_CLOUDFLARE;
  secondary_ = CFG_DNS_GOOGLE;
  source_ = "fallback";
  LOG_I("DNS", "Manager ready fallback=%s/%s", primary_.toString().c_str(),
        secondary_.toString().c_str());
}

void DnsManager::onStaGotIp() {
  IPAddress dns1 = WiFi.dnsIP(0);
  IPAddress dns2 = WiFi.dnsIP(1);
  if (dns1 != IPAddress(0, 0, 0, 0)) {
    primary_ = dns1;
    source_ = "upstream";
  } else {
    primary_ = CFG_DNS_CLOUDFLARE;
    source_ = "cloudflare";
  }
  if (dns2 != IPAddress(0, 0, 0, 0) && dns2 != primary_) {
    secondary_ = dns2;
  } else {
    secondary_ = (primary_ == CFG_DNS_CLOUDFLARE) ? CFG_DNS_GOOGLE : CFG_DNS_CLOUDFLARE;
  }
  applyToSoftApClients();
  LOG_I("DNS", "Updated primary=%s secondary=%s source=%s", primary_.toString().c_str(),
        secondary_.toString().c_str(), source_);
}

void DnsManager::onStaLost() {
  primary_ = CFG_DNS_CLOUDFLARE;
  secondary_ = CFG_DNS_GOOGLE;
  source_ = "fallback";
  applyToSoftApClients();
  LOG_W("DNS", "STA lost; using fallback DNS");
}

IPAddress DnsManager::primaryDns() {
  return primary_;
}
IPAddress DnsManager::secondaryDns() {
  return secondary_;
}
const char* DnsManager::sourceName() {
  return source_;
}

void DnsManager::applyToSoftApClients() {
  esp_netif_t* ap = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
  if (!ap) {
    LOG_W("DNS", "AP netif not found; SoftAP clients use captive/gateway DNS path");
    return;
  }

  DnsApplyContext ctx{};
  ctx.netif = ap;
  ctx.dns.ip.type = ESP_IPADDR_TYPE_V4;
  ctx.dns.ip.u_addr.ip4.addr = static_cast<uint32_t>(primary_);

  // DHCP start/stop schedules lwIP timeouts and therefore must run in the
  // TCP/IP thread. Calling it directly from Arduino's loop task causes the
  // IDF 5.x "Required to lock TCPIP core" assertion.
  esp_err_t err = esp_netif_tcpip_exec(applyDnsInTcpipContext, &ctx);
  if (err != ESP_OK) {
    LOG_W("DNS", "SoftAP DHCP DNS update failed 0x%x", static_cast<unsigned>(err));
    return;
  }
  LOG_I("DNS", "SoftAP DHCP DNS applied");
}
