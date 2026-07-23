# API (`/api/v1`)

All JSON responses may include `csrf` and `session` for the SoftAP admin session.

State-changing and sensitive backup requests require both headers:

```http
X-CSRF-Token: <token>
X-Session: <session>
```

Admin endpoints accept SoftAP clients (`192.168.4.0/24`) only.

## Endpoints

| Method | Path | Notes |
|--------|------|-------|
| GET | `/api/v1/status` | WiFi + system + health + OTA |
| GET | `/api/v1/scan?refresh=1` | Cached networks; `refresh=1` forces async rescan |
| GET | `/api/v1/clients` | SoftAP stations |
| GET | `/api/v1/settings` | AP settings (never returns STA password) |
| POST | `/api/v1/settings` | `{apSsid,apPass,deviceName,hostname}` |
| GET | `/api/v1/settings/backup` | Download versioned settings including credentials; requires session + CSRF |
| POST | `/api/v1/settings/restore` | Validate and restore a versioned backup |
| POST | `/api/v1/connect` | `{ssid,password}` |
| POST | `/api/v1/disconnect` | Drop STA / disable NAPT |
| POST | `/api/v1/reboot` | Restart |
| POST | `/api/v1/reset` | Factory wipe + restart |
| GET | `/api/v1/logs` | JSON text of RAM logs |
| GET | `/api/v1/logs?export=1` | Plaintext download |
| POST | `/api/v1/update` | Raw OTA image with required `X-File-Size` and recommended `X-SHA256` integrity header |
| WS | `/ws` | ~1 Hz status JSON |

After a successful OTA, the new slot remains pending until NanoExtend passes the
stable-boot window. It is then marked valid; otherwise the ESP32 bootloader can
return to the previous slot when rollback support is enabled.

## Errors

```json
{"ok":false,"error":"message","csrf":"...","session":"..."}
```

Common codes: `400` validation, `403` CSRF/ACL, `409` busy, `429` rate limited.
