import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import tailwindcss from "@tailwindcss/vite";

export default defineConfig({
  // Project site: https://fahimuntasin.github.io/NanoExtend/
  base: "/NanoExtend/",
  plugins: [react(), tailwindcss()],
  build: {
    target: "es2022",
    sourcemap: true,
  },
});
