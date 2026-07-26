# Contributing to NanoExtend

Thank you for helping make tiny networks better.

## Before you start

- Read the [architecture](docs/architecture.md) and [security policy](SECURITY.md).
- Search existing issues and discussions.
- Keep changes focused; firmware reliability has priority over feature count.

## Development setup

```bash
pipx install platformio==6.1.19
pio run -e esp32dev
cd website && npm ci && npm run build
```

The firmware target, platform, framework, and libraries are pinned in `platformio.ini`. Do not introduce floating versions.

## Pull requests

1. Create a branch from `main`.
2. Add tests or a hardware validation note for behavior changes.
3. Run firmware and website builds.
4. Update docs/OpenAPI/changelog when contracts change.
5. Complete the pull-request checklist.

## Firmware rules

- No blocking reconnect loops or long `delay()` calls.
- Never log credentials or tokens.
- Keep SoftAP available during STA recovery.
- Run lwIP timer/NAPT operations in TCP/IP context.
- Keep the release image below both OTA slot limits.
- Avoid unbounded allocation in steady-state paths.

## Website rules

- Meet WCAG AA contrast and keyboard navigation expectations.
- Keep installer flows deterministic and error messages actionable.
- Do not copy public product layouts or branding.
- Test at 320px, 768px, and desktop widths.

## Commit style

Use concise imperative subjects. Conventional prefixes are welcome: `feat:`, `fix:`, `docs:`, `ci:`, `refactor:`.
