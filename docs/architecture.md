# Architecture

## Runtime topology

```
Phone/Tablet ──WiFi── SoftAP (NanoExtend / 192.168.4.1)
   │                      │
   │ HTTP/WS              ├── Captive DNS (when offline)
   │                      ├── Async Web Server (/api/v1, /ws, SPA)
   │                      ├── WifiManager FSM
   │                      ├── NatRouter (lwIP NAPT)
   │                      ├── DnsManager
   │                      ├── OtaManager
   │                      └── System/Health/Logger/Storage(NVS)
   │
   └── after NAT ── STA ── Home AP ── Internet
```

## Modules

| Module | Ownership |
|--------|-----------|
| `main.cpp` | Boot order only |
| `wifi_manager` | SoftAP permanence, STA connect/reconnect, async scan cache, BSSID restore |
| `nat_router` | Version-aware NAPT enable/disable (`ip_napt_enable`) |
| `dns_manager` | Upstream/Cloudflare/Google DNS selection + SoftAP DHCP DNS |
| `captive_portal` | Wildcard DNS + OS captive redirects |
| `web_server` | SPA assets, REST, WebSocket, CSRF/rate limits, SoftAP ACL |
| `serial_admin` | USB-UART JSON admin for PC setup without SoftAP/phone |
| `ota` | Streamed dual-slot update with abort-on-failure |
| `storage` | Checksummed Preferences blob + crash record |
| `system` | Metrics, health probes, reset reason |
| `logger` | 200-entry circular RAM log |

## Boot sequence

1. Logger → Storage → System diagnostics
2. SoftAP always starts
3. Web server + captive portal start
4. If saved STA creds exist → auto connect
5. On STA got IP → DNS update → NAPT enable → health probes
6. Captive portal disables while sharing is healthy

## Partition map (4MB)

| Name | Offset | Size |
|------|--------|------|
| nvs | 0x9000 | 0x5000 |
| otadata | 0xE000 | 0x2000 |
| app0 (ota_0) | 0x10000 | 0x1C0000 |
| app1 (ota_1) | 0x1D0000 | 0x1C0000 |
| spiffs | 0x390000 | 0x60000 |
| coredump | 0x3F0000 | 0x10000 |

No separate factory app partition (would not fit useful OTA slots on 4MB).

## NAT compatibility

Pinned core **Arduino-ESP32 3.3.9** exposes:

```cpp
void ip_napt_enable(u32_t addr, int enable);
```

`NatRouter` wraps this API. Headers alone are insufficient; this project verified the symbols on pioarduino `55.03.39`.
