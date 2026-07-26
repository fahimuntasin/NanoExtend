# NanoExtend Website

React + Tailwind static site for GitHub Pages.

## Local development

```bash
npm ci
npm run typecheck
npm run build
npm run preview
```

Vite uses `base: "/NanoExtend/"` so production assets match
`https://fahimuntasin.github.io/NanoExtend/`.

## Installer firmware

The browser installer expects `public/firmware/manifest.json` plus the factory
image named by that manifest. The GitHub Pages workflow builds firmware with
PlatformIO and packages those artifacts into the site. Generated `.bin` files
are intentionally ignored by Git.

## Deployment

GitHub Actions workflow `.github/workflows/github-pages.yml` builds the firmware
artifacts, builds the site, and deploys to GitHub Pages. Web Serial requires
HTTPS (localhost is allowed during development).

SPA deep links use a `404.html` fallback generated from `index.html`.
