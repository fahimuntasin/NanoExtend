# Changelog

All notable changes follow [Keep a Changelog](https://keepachangelog.com/) and [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Changed
- Upgraded the animated NanoExtend mark with circuit-trace motion, signal arcs, and reduced-motion support.
- Refreshed landing-page branding, SEO assets, and installer firmware packaging for 1.0.1.
- Replaced Cloudflare Pages deployment with GitHub Pages hosting.

### Added
- Automatic GitHub Pages workflow that builds firmware installer artifacts and deploys the site.

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
