# USB dashboard

Classic ESP32 DevKits bridge USB through a UART chip (CH340/CP210x). That path is **serial**, not Ethernet — a browser cannot open `http://192.168.4.1` over the cable alone.

NanoExtend invents a **USB dashboard**: the same SoftAP-style UI (Home, Scan, Clients, Settings, System, Logs) talking to firmware over Web Serial.

## Requirements

- Firmware **1.0.4+**
- Data USB cable
- Chrome or Edge on desktop (HTTPS)
- Board already flashed with NanoExtend

## Flow

1. Open [USB Dashboard](https://fahimuntasin.github.io/NanoExtend/#/dashboard) (full page — not embedded in the landing scroll).
2. Click **Connect USB dashboard** and choose the USB serial port (CH340 / CP210x — not `ttyS*`).
3. The onboard LED plays a **double-pulse celebrate** pattern when `hello` succeeds.
4. Use the tabs: scan, connect, settings, logs, reboot, factory reset.
5. SoftAP at `http://192.168.4.1` remains available when you join the `NanoExtend` Wi-Fi.

OTA binary upload still uses SoftAP or the browser installer.

Theme: light (Claude-inspired paper) or dark — toggle in the dashboard bar.

## Protocol (for tools)

Baud: **115200**. Prefer the USB-UART bridge (CH340 / CP210x / FTDI). On Linux, Chrome may also list motherboard ports like `ttyS0` — those are **not** the ESP32. The website filters the chooser to common USB serial vendor IDs.

| Direction | Format |
|-----------|--------|
| Host → device | `NE>{"v":1,"id":1,"cmd":"ping"}\n` |
| Device → host | `NE{"v":1,"id":1,"ok":true,"result":{...}}\n` |

Commands: `ping` / `hello`, `celebrate`, `status`, `scan`, `connect`, `disconnect`, `settings_get`, `settings_set`, `clients`, `logs`, `health`, `factory_reset`, `reboot`.

Ignore any serial line that does not start with `NE{`.

## Security

Physical USB access is treated like SoftAP admin access. Rate limits apply. Do not leave an unlocked PC attached to a provisioned travel router on an untrusted network.
