# Developer Guide

## Repository layout

- `src/`, `include/`, `partitions/`: firmware
- `assets/web/`: embedded device dashboard
- `website/`: React + Tailwind Cloudflare Pages site and installer
- `api/openapi.yaml`: API contract
- `.github/workflows/`: CI, release, and deployment
- `docs/`: user and architecture documentation

## Commands

```bash
pio run -e esp32dev
cd website
npm ci
npm run typecheck
npm run build
```

## Release process

Update `CHANGELOG.md`, bump semantic version/build flag, push an annotated `vX.Y.Z` tag, and let the release workflow build/checksum/attach both factory and OTA binaries plus the manifest.
