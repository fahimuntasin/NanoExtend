# Changelog

All notable changes follow [Keep a Changelog](https://keepachangelog.com/) and [Semantic Versioning](https://semver.org/).

## [Unreleased]

## [1.0.4] - 2026-07-26

### Added
- Full-page USB dashboard route (`#/dashboard`) with Claude-inspired light theme and theme switcher.
- Live SVG atmosphere art, white action buttons, and Made by fahimuntasin.com credit.
- In-site Docs and Changelog pages.
- Onboard LED celebrate double-pulse on USB `hello` / connect (`StatusLed`).

### Changed
- Landing page links to the dedicated dashboard instead of embedding USB setup mid-page.

## [1.0.3] - 2026-07-26

### Added
- Full USB dashboard (Home / Scan / Clients / Settings / System / Logs) over Web Serial.
- SerialAdmin commands: `clients`, `logs`, `health`, `factory_reset`.

### Changed
- USB Setup panel upgraded to SoftAP-style dashboard UX.

## [1.0.2] - 2026-07-26

### Added
- `SerialAdmin` firmware channel over USB-UART for scan/connect/settings without SoftAP.
- Landing-page USB Setup panel using Web Serial at 115200 baud.
- `docs/usb-setup.md` protocol and security notes.
- Real ESP32 product-photo showcase on the README and landing Hardware section.

### Changed
- Installer completion copy points to USB Setup as well as SoftAP onboarding.

## [1.0.1] - 2026-07-23

### Fixed
- Use GitHub Releases as the dashboard's working default update server.
- Keep custom update-server support while avoiding an undeployed-site dependency.

### Changed
- Updated GitHub Actions to current Node 24-compatible major versions.

## [1.0.0] - 2026-07-23

### Added
- React + Tailwind Cloudflare Pages website and Web Serial installer.
- SHA-256 OTA validation and delayed stable-boot confirmation.
- Settings backup and restore APIs and dashboard controls.
- OpenAPI contract, CI/CD, release packaging, and community files.
- Original animated NanoExtend brand system.
- ESP32 SoftAP + STA routing with lwIP NAPT.
- DHCP/DNS management, captive portal, reconnect recovery, and health probes.
- Local responsive dashboard, realtime WebSocket status, logs, and local OTA.
- Preferences persistence, factory reset, crash diagnostics, and dual OTA partitions.
