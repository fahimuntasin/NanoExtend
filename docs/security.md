# Security Architecture

## Trust boundaries

1. Untrusted upstream Wi-Fi/Internet
2. NanoExtend STA/NAPT path
3. Trusted SoftAP management subnet
4. Physical USB/flash access

State-changing APIs require SoftAP origin, active session, and CSRF token. Inputs are bounded and dynamic UI content is escaped.

## OTA

Local OTA validates size and SHA-256 and uses dual slots. SHA-256 detects corruption but is not a signature. Authenticity requires a signed release manifest or ESP-IDF secure-boot signature verification.

## Storage

Preferences records include schema, generation, and checksum. Credentials are excluded from logs. Backups intentionally contain secrets and use `Cache-Control: no-store`.
