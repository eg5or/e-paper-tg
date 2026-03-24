import { Pool } from "pg";
import { config } from "./config";

export const pool = new Pool({
  connectionString: config.databaseUrl
});

export async function initDb() {
  await pool.query(`
    CREATE TABLE IF NOT EXISTS devices (
      id TEXT PRIMARY KEY,
      name TEXT NOT NULL,
      api_key TEXT NOT NULL,
      enabled BOOLEAN NOT NULL DEFAULT TRUE,
      created_at BIGINT NOT NULL
    );
  `);

  await pool.query(`
    CREATE TABLE IF NOT EXISTS commands (
      id BIGSERIAL PRIMARY KEY,
      device_id TEXT NOT NULL REFERENCES devices(id) ON DELETE CASCADE,
      type TEXT NOT NULL CHECK (type IN ('text', 'image')),
      from_name TEXT NOT NULL,
      text_payload TEXT,
      image_data BYTEA,
      image_preview_png BYTEA,
      image_width INTEGER,
      image_height INTEGER,
      created_at BIGINT NOT NULL,
      delivered_at BIGINT,
      status TEXT NOT NULL CHECK (status IN ('pending', 'delivered')) DEFAULT 'pending'
    );
  `);

  await pool.query(`
    ALTER TABLE commands
    ADD COLUMN IF NOT EXISTS image_preview_png BYTEA;
  `);

  await pool.query(`
    CREATE INDEX IF NOT EXISTS idx_commands_device_status_id
    ON commands(device_id, status, id);
  `);
}
