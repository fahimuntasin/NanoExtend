# Flashing and OTA

## Wiring

USB-powered ESP32 DevKit V1 only — no extra wiring required. CH340/CP210x USB-UART on `/dev/ttyUSB0`.

User must be in the `dialout` group:

```bash
sudo usermod -aG dialout $USER
```

## First flash (USB)

```bash
cd NanoExtend
pio run -e esp32dev -t upload
pio device monitor -b 115200
```

Verified hardware during implementation:

- Chip: ESP32-D0WD-V3 (rev v3.1)
- Flash: 4MB
- Port: `/dev/ttyUSB0`
- USB ID: QinHeng CH340 (`1a86:7523`)

## Web OTA

1. Join SoftAP and open dashboard → **OTA**.
2. Upload the built `.pio/build/esp32dev/firmware.bin`.
3. Progress streams over `/ws`.
4. On success the device reboots into the new slot.
5. On failure the previous slot remains bootable.

## Partition notes

Dual OTA slots are each `0x1C0000` (~1.75MB). Current image is ~1.20MB.
