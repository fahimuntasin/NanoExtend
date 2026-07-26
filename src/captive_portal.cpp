#include "captive_portal.h"

#include "config.h"
#include "logger.h"

DNSServer CaptivePortal::dns_;
bool CaptivePortal::active_ = false;
IPAddress CaptivePortal::apIp_ = CFG_AP_IP;

void CaptivePortal::begin(const IPAddress& apIp) {
  apIp_ = apIp;
  dns_.setErrorReplyCode(DNSReplyCode::NoError);
  dns_.start(CFG_CAPTIVE_DNS_PORT, "*", apIp_);
  active_ = true;
  LOG_I("Captive", "DNS captive portal started -> %s", apIp_.toString().c_str());
}

void CaptivePortal::loop() {
  if (active_)
    dns_.processNextRequest();
}

void CaptivePortal::setActive(bool active) {
  if (active == active_)
    return;
  active_ = active;
  if (active_) {
    dns_.start(CFG_CAPTIVE_DNS_PORT, "*", apIp_);
    LOG_I("Captive", "Enabled");
  } else {
    dns_.stop();
    LOG_I("Captive", "Disabled (internet sharing active)");
  }
}

bool CaptivePortal::isActive() {
  return active_;
}

bool CaptivePortal::handleCaptiveRequest(const String& host, const String& path) {
  // Detect OS captive portal probes.
  if (path.indexOf("generate_204") >= 0 || path.indexOf("gen_204") >= 0 ||
      path.indexOf("hotspot-detect") >= 0 || path.indexOf("connecttest") >= 0 ||
      path.indexOf("ncsi.txt") >= 0 || path.indexOf("captive") >= 0) {
    return true;
  }
  if (host.length() == 0)
    return false;
  if (host == apIp_.toString() || host == "nanoextend.local" || host.startsWith("nanoextend")) {
    return false;
  }
  // Unknown hosts while captive is active should redirect to dashboard.
  return active_;
}
