# FAQ

## Why only one or two clients?

NanoExtend optimizes for stable travel-router use on a resource-constrained ESP32, not access-point throughput.

## Why does my phone disconnect once after selecting upstream Wi-Fi?

ESP32 AP+STA shares one radio channel. Joining an upstream network on another channel can briefly move the SoftAP channel.

## Is this a Wi-Fi repeater?

It is a routed/NAT extender, not a transparent layer-2 repeater.

## Does it work without internet?

Yes. The dashboard, AP, settings, logs, backup/restore, and local OTA remain available.

## Are backups encrypted?

No. Backups contain Wi-Fi credentials in JSON and must be stored securely.
