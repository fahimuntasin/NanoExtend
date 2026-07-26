# Getting Started

## What you need

- ESP32 DevKit V1 with 4MB flash
- Data-capable USB cable
- Chrome/Edge desktop for the browser installer, or PlatformIO

## Browser installation

1. Open the NanoExtend website over HTTPS.
2. Select **Connect and install**.
3. Choose the CH340/CP210x serial port.
4. Wait for checksum verification and flashing.
5. Either:
   - Open **USB Setup** on the same site and configure upstream Wi-Fi over the cable (no phone), or
   - Join `NanoExtend` with `changeme123`, open `http://192.168.4.1`, change the AP password, and connect upstream Wi-Fi.

See [USB setup](usb-setup.md) for the serial protocol and PC-only flow.

The AP can briefly disconnect when the ESP32 radio moves to the upstream channel. Rejoin once; routing then remains active.
