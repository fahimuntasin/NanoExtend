<div align="center">
  <img src="brand/logo.svg" alt="NanoExtend logo" width="112" />
  <h1>NanoExtend</h1>
  <p><strong>Tiny ESP32. Big Network.</strong></p>
  <p>Turn an ESP32 DevKit V1 into a polished, self-healing Wi-Fi NAT travel router.</p>

[![CI](https://github.com/fahimuntasin/NanoExtend/actions/workflows/ci.yml/badge.svg)](https://github.com/fahimuntasin/NanoExtend/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/fahimuntasin/NanoExtend)](https://github.com/fahimuntasin/NanoExtend/releases)
[![License: MIT](https://img.shields.io/badge/license-MIT-white.svg)](LICENSE)
[![Hardware: ESP32](https://img.shields.io/badge/hardware-ESP32-black.svg)](docs/installation.md)

[Install](#install-in-five-minutes) · [Documentation](docs/getting-started.md) · [API](api/openapi.yaml) · [Roadmap](ROADMAP.md) · [Contributing](CONTRIBUTING.md)
</div>

---

## Why NanoExtend?

Typical embedded networking projects stop at a serial console and a working packet path. NanoExtend treats routing, recovery, installation, updates, security, documentation, and visual design as one product.

- **Real routing:** SoftAP + STA, lwIP NAPT, DHCP, DNS forwarding
- **Self-healing:** exponential reconnect, BSSID recovery, health probes
- **Local-first UX:** responsive black-and-white dashboard, WebSocket telemetry
- **Safe lifecycle:** checksummed settings, backup/restore, factory reset, crash diagnostics
- **Verified OTA:** SHA-256 uploads, equal dual slots, stable-boot confirmation
- **Five-minute installer:** React/Tailwind Cloudflare site with Web Serial flashing
- **Open ecosystem:** OpenAPI, CI/CD, release artifacts, security and contribution policies

## Install in five minutes

### Browser installer

Open the NanoExtend website in Chrome or Edge over HTTPS, select **Connect and install**, choose the ESP32 serial port, and wait for verification. The installer detects the chip and validates the firmware SHA-256 before flashing.

### PlatformIO

```bash
pipx install platformio==6.1.19
git clone https://github.com/fahimuntasin/NanoExtend.git
cd nanoextend
pio run -e esp32dev -t upload
```

Then join:

```text
SSID: NanoExtend
Password: changeme123
Dashboard: http://192.168.4.1
```

Change the default AP password immediately.

## Architecture

```text
Phone / laptop
     │ Wi-Fi + DHCP
     ▼
NanoExtend SoftAP (192.168.4.1)
     ├── Local dashboard + REST / WebSocket
     ├── Captive portal + DNS manager
     ├── Settings / OTA / recovery
     └── lwIP NAPT
             │ ESP32 STA
             ▼
       Upstream Wi-Fi → Internet
```

See [Architecture](docs/architecture.md) for module ownership, state transitions, partition layout, and NAT compatibility.

## Repository

```text
src/ + include/       ESP32 firmware modules
assets/web/           embedded local dashboard
partitions/           4MB dual-OTA layout
website/              React + Tailwind landing site and Web Serial installer
api/openapi.yaml      versioned device API contract
docs/                 user, security, OTA, architecture, developer docs
.github/workflows/     CI, semantic release artifacts, Cloudflare Pages
brand/                original NanoExtend visual identity
```

## Exact toolchain

| Component | Pin |
|---|---|
| PlatformIO Core | `6.1.19` |
| pioarduino platform | `55.03.39` |
| Arduino-ESP32 | `3.3.9` |
| ESP-IDF libraries | `5.5.4` |
| ESPAsyncWebServer | `v3.6.0` |
| AsyncTCP | `v3.3.2` |
| ArduinoJson | `7.3.0` |

No floating firmware dependencies are used.

## Measured footprint

Current release build on ESP32 DevKit V1:

- RAM: about **75 KB / 320 KB** at link time
- Firmware: about **1.22 MB / 1.75 MB** per OTA slot
- Flash: **4 MB**, no PSRAM
- Intended clients: **1–2**

## Development

```bash
# Firmware
pio run -e esp32dev

# Website
cd website
npm ci
npm run typecheck
npm run build
```

Release tags (`vX.Y.Z`) produce factory, OTA, ELF, SHA256SUMS, and manifest assets. Cloudflare Pages deployment requires `CLOUDFLARE_API_TOKEN`, `CLOUDFLARE_ACCOUNT_ID`, and optional `CLOUDFLARE_PROJECT_NAME` repository configuration.

## Documentation

- [Getting started](docs/getting-started.md)
- [Installation and flashing](docs/installation.md)
- [Dashboard](docs/dashboard-guide.md)
- [OTA and rollback](docs/ota-guide.md)
- [REST/WebSocket API](docs/api.md) and [OpenAPI](api/openapi.yaml)
- [Troubleshooting](docs/troubleshooting.md)
- [FAQ](docs/faq.md)
- [Security architecture](docs/security.md)
- [Developer guide](docs/developer-guide.md)
- [Roadmap](ROADMAP.md)

## Security

Please read [SECURITY.md](SECURITY.md). Never publish credentials, private SSIDs, tokens, or unredacted network logs. Use private vulnerability reporting for security issues.

## Community

Contributions are welcome. Read [CONTRIBUTING.md](CONTRIBUTING.md), follow the [Code of Conduct](CODE_OF_CONDUCT.md), and use Discussions for support and ideas.

## Known limitations

- NanoExtend is a routed NAT extender, not a transparent layer-2 repeater.
- One ESP32 radio means the SoftAP follows the upstream channel and can briefly disconnect during channel changes.
- SHA-256 verifies integrity, not publisher identity; signed update manifests and production Secure Boot provisioning remain roadmap work.
- The project prioritizes reliable operation for one or two clients over throughput.

## License

[MIT](LICENSE) © NanoExtend contributors.

## Credits

Built on Espressif Arduino-ESP32/ESP-IDF/lwIP, pioarduino, ESP32Async, ArduinoJson, React, Tailwind CSS, and esptool-js.
