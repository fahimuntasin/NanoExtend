# Troubleshooting

## SoftAP not visible

- Confirm USB power and serial logs: `[WiFi] Starting AP...`
- Hold BOOT while resetting if flash failed mid-way, then re-upload.

## Dashboard does not open

- Connect to `NanoExtend`, then open `http://192.168.4.1`
- Captive portal only redirects while STA is offline / sharing inactive
- Clear phone Wi-Fi “auto join” captive cache if needed

## Connected to home Wi-Fi but clients have no internet

1. Check serial: `[NAT] Enabled`
2. Status card must show NAT on / sharing
3. Health detail should not be all-fail
4. Confirm upstream allows more clients / no AP isolation issues on the home router
5. Reboot NanoExtend after first successful connect if DHCP leases were issued before DNS update

## Scan returns empty / stale

- Results cache for 15 seconds
- Use **Refresh**; only one scan job runs at a time

## OTA fails

- Image must fit free OTA slot (`ESP.getFreeSketchSpace()`)
- Upload `firmware.bin` (app image), not `firmware.factory.bin`, for web OTA
- Ensure CSRF headers are present (dashboard handles this)

## NVS / brownout concerns

- Settings writes are checksummed; invalid blobs fall back to defaults
- Avoid yanking power during OTA; failed OTA aborts and keeps previous image

## Permission denied on `/dev/ttyUSB0`

```bash
sudo usermod -aG dialout $USER
# re-login, then retry upload
```

## Build / NAT symbols

If another platform pin is tried and `ip_napt_enable` is missing at link time, stop and switch back to pioarduino `55.03.39` (Arduino 3.3.9). Do not mix headers from one core with libs from another.
