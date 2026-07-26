#pragma once

#include <Arduino.h>

#define NANOEXTEND_NAME "NanoExtend"
#ifndef NANOEXTEND_FW_VERSION
#define NANOEXTEND_FW_VERSION "1.0.4"
#endif

// SoftAP defaults
#define CFG_AP_SSID_DEFAULT "NanoExtend"
#define CFG_AP_PASS_DEFAULT "changeme123"
#define CFG_AP_IP IPAddress(192, 168, 4, 1)
#define CFG_AP_GW IPAddress(192, 168, 4, 1)
#define CFG_AP_MASK IPAddress(255, 255, 255, 0)
#define CFG_AP_MAX_CLIENTS 2
#define CFG_AP_CHANNEL 1
#define CFG_AP_HIDDEN 0

// DNS fallbacks
#define CFG_DNS_CLOUDFLARE IPAddress(1, 1, 1, 1)
#define CFG_DNS_GOOGLE IPAddress(8, 8, 8, 8)

// Storage
#define CFG_NVS_NAMESPACE "nanoextend"
#define CFG_STORAGE_SCHEMA 2

// Timing
#define CFG_SCAN_CACHE_MS 15000UL
#define CFG_WS_INTERVAL_MS 1000UL
#define CFG_RECONNECT_MIN_MS 1000UL
#define CFG_RECONNECT_MAX_MS 60000UL
#define CFG_HEALTH_INTERVAL_MS 15000UL
#define CFG_STABLE_BOOT_MS 30000UL
#define CFG_CONNECT_TIMEOUT_MS 20000UL

// Limits
#define CFG_SSID_MAX 32
#define CFG_PASS_MAX 63
#define CFG_PASS_MIN 8
#define CFG_DEVICE_NAME_MAX 32
#define CFG_HOSTNAME_MAX 32
#define CFG_SCAN_MAX 24
#define CFG_LOG_MAX 200
#define CFG_LOG_LINE_MAX 96
#define CFG_JSON_BUF 3072
#define CFG_RATE_LIMIT_WINDOW_MS 1000UL
#define CFG_RATE_LIMIT_MAX 8

// Memory budgets (soft targets)
#define CFG_HEAP_WARN_BYTES 60000UL
#define CFG_RAM_TARGET_BYTES (220UL * 1024UL)

// Admin session
#define CFG_SESSION_TTL_MS (30UL * 60UL * 1000UL)
#define CFG_CSRF_LEN 24

// Health probes
#define CFG_HEALTH_DNS_HOST "one.one.one.one"
#define CFG_HEALTH_HTTP_URL "http://connectivitycheck.gstatic.com/generate_204"
#define CFG_HEALTH_HTTPS_HOST "1.1.1.1"
#define CFG_HEALTH_HTTPS_PORT 443
#define CFG_HEALTH_TIMEOUT_MS 4000UL

// Captive portal
#define CFG_CAPTIVE_DNS_PORT 53
