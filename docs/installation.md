# Installation

## Web installer

The React website uses Web Serial and `esptool-js`. It verifies the release manifest size and SHA-256 before writing the merged factory image at address `0x0`.

## PlatformIO

```bash
pipx install platformio==6.1.19
git clone https://github.com/fahimuntasin/NanoExtend.git
cd nanoextend
pio run -e esp32dev -t upload
```

## Release binaries

- `nanoextend-<version>.factory.bin`: first USB/web installation at `0x0`.
- `nanoextend-<version>.ota.bin`: dashboard OTA update only.
- `manifest.json`: version, size, SHA-256, chip, release notes.
