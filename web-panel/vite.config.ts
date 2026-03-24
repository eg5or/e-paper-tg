import { defineConfig } from "vite";
import react from "@vitejs/plugin-react-swc";
import path from "node:path";
import fs from "node:fs";
import dotenv from "dotenv";

const envCandidates = [
  path.resolve(process.cwd(), ".env.web"),
  path.resolve(process.cwd(), "../.env.web")
];

for (const candidate of envCandidates) {
  if (fs.existsSync(candidate)) {
    dotenv.config({ path: candidate });
    break;
  }
}

const backendPort = Number(process.env.BACKEND_PORT || 3001);

export default defineConfig({
  plugins: [react()],
  resolve: {
    alias: {
      "@": path.resolve(__dirname, "src/client")
    }
  },
  server: {
    port: 5173,
    proxy: {
      "/api": `http://localhost:${backendPort}`,
      "/health": `http://localhost:${backendPort}`
    }
  },
  build: {
    outDir: "dist/client"
  }
});
