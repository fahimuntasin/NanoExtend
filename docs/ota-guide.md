# OTA Guide

NanoExtend uses equal `ota_0` and `ota_1` partitions. Uploads are streamed, size-limited, and SHA-256 checked when the dashboard supplies `X-SHA256`.

After reboot, a new image remains pending until the stable-boot window passes. The firmware then calls `esp_ota_mark_app_valid_cancel_rollback()`.

## Dashboard update

1. Build or download the `.ota.bin` application image.
2. Open **OTA** in the local dashboard.
3. Select the image. The browser computes SHA-256 before upload.
4. Do not remove power until the device restarts.

A production device can additionally enable ESP32 Secure Boot and Flash Encryption; these require irreversible provisioning decisions and are intentionally not enabled by development builds.
