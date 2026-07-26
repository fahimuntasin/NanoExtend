# USB setup

Classic ESP32 DevKits bridge USB through a UART chip (CH340/CP210x). That path is **serial**, not Ethernet — a browser cannot open `http://192.168.4.1` over the cable alone.

NanoExtend solves “no phone” setup with a **USB serial admin** channel plus the website **USB Setup** panel (Web Serial).

## Requirements

- Firmware **1.0.2+**
- Data USB cable
- Chrome or Edge on desktop (HTTPS)
- Board already flashed with NanoExtend

## Flow

1. Open [USB Setup](https://fahimuntasin.github.io/NanoExtend/#usb-setup).
2. Click **Connect USB and setup** and choose the serial port.
3. Wait for `hello` / firmware version.
4. **Rescan**, pick an upstream SSID, enter the password, connect.
5. Watch **STA IP** appear in the status strip.

The SoftAP (`NanoExtend` / your AP password) still runs. Join it from any Wi-Fi client when you want the full on-device dashboard at `http://192.168.4.1`.

## Protocol (for tools)

Baud: **115200**. Line-oriented JSON with a fixed prefix so logs do not collide.

| Direction | Format |
|-----------|--------|
| Host → device | `NE>{"v":1,"id":1,"cmd":"ping"}\n` |
| Device → host | `NE{"v":1,"id":1,"ok":true,"result":{...}}\n` |

Commands: `ping` / `hello`, `status`, `scan`, `connect`, `disconnect`, `settings_get`, `settings_set`, `reboot`.

Ignore any serial line that does not start with `NE{`.

## Security

Physical USB access is treated like SoftAP admin access. Rate limits apply. Do not leave an unlocked PC attached to a provisioned travel router on an untrusted network.
