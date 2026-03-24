import fs from "node:fs";
import path from "node:path";
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

const nodeEnv = process.env.NODE_ENV || "development";
const defaultPort = nodeEnv === "production" ? 3028 : 3001;
const defaultDbHost = nodeEnv === "production" ? "postgres" : "localhost";
const dbHost = process.env.DB_HOST || defaultDbHost;
const dbPort = Number(process.env.DB_PORT || 5432);
const dbName = process.env.POSTGRES_DB || "paper";
const dbUser = process.env.POSTGRES_USER || "paper";
const dbPassword = process.env.POSTGRES_PASSWORD || "change_me";
const defaultDbUrl = `postgresql://${encodeURIComponent(dbUser)}:${encodeURIComponent(dbPassword)}@${dbHost}:${dbPort}/${dbName}`;

export const config = {
  backendPort: Number(process.env.BACKEND_PORT || defaultPort),
  webUsername: process.env.WEB_USERNAME || "admin",
  webPassword: process.env.WEB_PASSWORD || "admin",
  webSessionSecret: process.env.WEB_SESSION_SECRET || "paper-web-session-secret",
  databaseUrl: process.env.DATABASE_URL || defaultDbUrl,
  webTargetWidth: Number(process.env.WEB_TARGET_WIDTH || 400),
  webTargetHeight: Number(process.env.WEB_TARGET_HEIGHT || 300),
  webMaxOutputBytes: Number(process.env.WEB_MAX_OUTPUT_BYTES || 20000),
  nodeEnv
} as const;
