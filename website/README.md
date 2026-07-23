# NanoExtend Website

Production React + TypeScript + Tailwind CSS site for NanoExtend. It includes the landing page, original animated brand, responsive dashboard preview, documentation links, and an `esptool-js` Web Serial installer.

## Development

```bash
npm ci
npm run dev
```

## Verification

```bash
npm run typecheck
npm run format:check
npm run build
```

The browser installer expects `public/firmware/manifest.json` plus the factory image named by that manifest. Release and Cloudflare workflows generate these from the pinned PlatformIO firmware build. Generated `.bin` files are intentionally ignored by Git.

## Deployment

Cloudflare Pages uses `wrangler.toml` and the repository workflow. Configure `CLOUDFLARE_API_TOKEN`, `CLOUDFLARE_ACCOUNT_ID`, and optionally `CLOUDFLARE_PROJECT_NAME`. Web Serial requires HTTPS (localhost is allowed during development).

Security headers and CORS for release manifests are defined in `public/_headers`.
