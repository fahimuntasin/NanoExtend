# Security Policy

## Supported versions

| Version | Supported |
|---|---|
| Latest stable release | Yes |
| Older releases | Critical fixes only when practical |
| Development snapshots | No guarantee |

## Reporting a vulnerability

Do not open a public issue. Use GitHub private vulnerability reporting for this repository. Include affected version, hardware, impact, reproduction steps, and suggested mitigation if known.

We aim to acknowledge reports within 5 business days and provide a remediation decision within 14 days.

## Security model

- The management API is restricted to the SoftAP subnet.
- State-changing operations require a session and CSRF token.
- Credentials are stored in ESP32 NVS and are never included in logs.
- OTA images are size-checked and SHA-256 verified.
- Dual OTA slots preserve the previous image until stable-boot confirmation.
- Settings backups contain secrets and must be protected by the user.

## Important limitations

NanoExtend is a travel-router project for trusted local environments, not an enterprise firewall. The default AP password must be changed. Firmware authenticity ultimately requires signed-image verification and Secure Boot/Flash Encryption provisioning, which are hardware deployment choices and are not enabled by default development images.
