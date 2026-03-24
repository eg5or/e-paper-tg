import fs from "node:fs";
import path from "node:path";
import express, { type Request, type Response } from "express";
import session from "express-session";
import multer from "multer";
import { config } from "./config";
import { requireAuth } from "./auth";
import { processImageToRaw } from "./image";
import { initDb, pool } from "./db";

const app = express();
const upload = multer({ storage: multer.memoryStorage() });

app.use(express.json({ limit: "8mb" }));
app.use(express.urlencoded({ extended: true, limit: "8mb" }));
app.use(
  session({
    secret: config.webSessionSecret,
    resave: false,
    saveUninitialized: false,
    cookie: { httpOnly: true, sameSite: "lax" }
  })
);

app.get("/health", (_req, res) => {
  res.json({ ok: true, service: "paper-web" });
});

// ---------- Auth ----------
app.get("/api/auth/me", (req, res) => {
  res.json({ ok: true, isAuthed: Boolean(req.session.isAuthed) });
});

app.post("/api/auth/login", (req, res) => {
  const { username, password } = req.body as { username?: string; password?: string };
  if (username === config.webUsername && password === config.webPassword) {
    req.session.isAuthed = true;
    return res.json({ ok: true });
  }
  return res.status(401).json({ ok: false, message: "Invalid credentials" });
});

app.post("/api/auth/logout", (req, res) => {
  req.session.destroy(() => res.json({ ok: true }));
});

// ---------- Panel APIs ----------
app.get("/api/panel/devices", requireAuth, (_req, res) => {
  void (async () => {
    const result = await pool.query(
      "SELECT id, name, api_key AS \"apiKey\", enabled, created_at AS \"createdAt\" FROM devices ORDER BY created_at DESC"
    );
    res.json({ ok: true, devices: result.rows });
  })().catch((err) => {
    res.status(500).json({ ok: false, message: err.message });
  });
});

app.post("/api/panel/devices", requireAuth, (req, res) => {
  const { id, name, apiKey, enabled } = req.body as {
    id?: string;
    name?: string;
    apiKey?: string;
    enabled?: boolean;
  };
  const deviceId = (id || "").trim();
  if (!deviceId) return res.status(400).json({ ok: false, message: "Device ID required" });

  void (async () => {
    const now = Math.floor(Date.now() / 1000);
    const finalName = (name || "").trim() || deviceId;
    const finalApiKey = (apiKey || "").trim();
    if (!finalApiKey) return res.status(400).json({ ok: false, message: "API key required" });
    await pool.query(
      `
      INSERT INTO devices (id, name, api_key, enabled, created_at)
      VALUES ($1, $2, $3, $4, $5)
      ON CONFLICT (id)
      DO UPDATE SET name = EXCLUDED.name, api_key = EXCLUDED.api_key, enabled = EXCLUDED.enabled
      `,
      [deviceId, finalName, finalApiKey, Boolean(enabled), now]
    );
    res.json({
      ok: true,
      device: { id: deviceId, name: finalName, apiKey: finalApiKey, enabled: Boolean(enabled), createdAt: now }
    });
  })().catch((err) => {
    res.status(500).json({ ok: false, message: err.message });
  });
});

app.delete("/api/panel/devices/:id", requireAuth, (req, res) => {
  const deviceId = req.params.id;
  void (async () => {
    await pool.query("DELETE FROM devices WHERE id = $1", [deviceId]);
    res.json({ ok: true });
  })().catch((err) => {
    res.status(500).json({ ok: false, message: err.message });
  });
});

app.get("/api/panel/stats", requireAuth, (_req, res) => {
  void (async () => {
    const [{ rows: pendingRows }, { rows: deliveredRows }, { rows: deviceRows }] = await Promise.all([
      pool.query("SELECT COUNT(*)::int AS count FROM commands WHERE status = 'pending'"),
      pool.query("SELECT COUNT(*)::int AS count FROM commands WHERE status = 'delivered'"),
      pool.query("SELECT COUNT(*)::int AS count FROM devices")
    ]);
    res.json({
      ok: true,
      stats: {
        pending: pendingRows[0].count,
        delivered: deliveredRows[0].count,
        devices: deviceRows[0].count
      }
    });
  })().catch((err) => {
    res.status(500).json({ ok: false, message: err.message });
  });
});

app.get("/api/panel/queue", requireAuth, (_req, res) => {
  void (async () => {
    const result = await pool.query(
      `
      SELECT
        id,
        device_id AS "deviceId",
        type,
        from_name AS "fromName",
        text_payload AS "textPayload",
        image_width AS "imageWidth",
        image_height AS "imageHeight",
        (image_preview_png IS NOT NULL) AS "hasPreview",
        created_at AS "createdAt"
      FROM commands
      WHERE status = 'pending'
      ORDER BY id ASC
      `
    );
    res.json({ ok: true, queue: result.rows });
  })().catch((err) => {
    res.status(500).json({ ok: false, message: err.message });
  });
});

app.get("/api/panel/queue/:id/preview", requireAuth, (req, res) => {
  const id = Number(req.params.id);
  if (!Number.isFinite(id) || id <= 0) {
    return res.status(400).json({ ok: false, message: "Invalid command id" });
  }

  void (async () => {
    const result = await pool.query("SELECT image_preview_png FROM commands WHERE id = $1", [id]);
    if (!result.rowCount) return res.status(404).json({ ok: false, message: "Command not found" });
    const preview = result.rows[0].image_preview_png as Buffer | null;
    if (!preview) return res.status(404).json({ ok: false, message: "Preview not found" });
    res.setHeader("Cache-Control", "no-store");
    return res.type("image/png").send(preview);
  })().catch((err) => {
    res.status(500).json({ ok: false, message: err.message });
  });
});

app.post("/api/panel/image/preview", requireAuth, upload.single("image"), async (req, res) => {
  try {
    const file = req.file;
    if (!file) return res.status(400).json({ ok: false, message: "Image file required" });
    const processed = await processImageToRaw(file.buffer);
    res.setHeader("Cache-Control", "no-store");
    res.setHeader("X-Image-Width", String(processed.width));
    res.setHeader("X-Image-Height", String(processed.height));
    return res.type("image/png").send(processed.previewPng);
  } catch (error) {
    const msg = error instanceof Error ? error.message : "unknown error";
    return res.status(500).json({ ok: false, message: msg });
  }
});

app.post("/api/panel/send/text", requireAuth, (req, res) => {
  const { deviceId, text, fromName } = req.body as { deviceId?: string; text?: string; fromName?: string };
  void (async () => {
    if (!(text || "").trim()) return res.status(400).json({ ok: false, message: "Text is empty" });
    const exists = await pool.query("SELECT id FROM devices WHERE id = $1 AND enabled = TRUE", [deviceId]);
    if (!exists.rowCount) return res.status(400).json({ ok: false, message: "Unknown or disabled device" });

    const now = Math.floor(Date.now() / 1000);
    const insert = await pool.query(
      `INSERT INTO commands (device_id, type, from_name, text_payload, created_at, status)
       VALUES ($1, 'text', $2, $3, $4, 'pending')
       RETURNING id`,
      [deviceId, (fromName || "Web").trim() || "Web", text, now]
    );

    res.json({
      ok: true,
      command: {
        id: insert.rows[0].id,
        deviceId,
        type: "text",
        fromName: (fromName || "Web").trim() || "Web",
        text,
        createdAt: now,
        status: "pending"
      }
    });
  })().catch((err) => {
    res.status(500).json({ ok: false, message: err.message });
  });
});

app.post("/api/panel/send/image", requireAuth, upload.single("image"), async (req, res) => {
  try {
    const { deviceId, fromName } = req.body as { deviceId?: string; fromName?: string };
    const file = req.file;
    if (!file) return res.status(400).json({ ok: false, message: "Image file required" });

    const processed = await processImageToRaw(file.buffer);
    const exists = await pool.query("SELECT id FROM devices WHERE id = $1 AND enabled = TRUE", [deviceId]);
    if (!exists.rowCount) return res.status(400).json({ ok: false, message: "Unknown or disabled device" });
    const now = Math.floor(Date.now() / 1000);
    const insert = await pool.query(
      `INSERT INTO commands (
        device_id, type, from_name, image_data, image_preview_png, image_width, image_height, created_at, status
      ) VALUES ($1, 'image', $2, $3, $4, $5, $6, $7, 'pending')
      RETURNING id`,
      [
        deviceId,
        (fromName || "Web").trim() || "Web",
        processed.epd,
        processed.previewPng,
        processed.width,
        processed.height,
        now
      ]
    );
    res.json({
      ok: true,
      command: {
        id: insert.rows[0].id,
        deviceId,
        type: "image",
        fromName: (fromName || "Web").trim() || "Web",
        imageWidth: processed.width,
        imageHeight: processed.height,
        createdAt: now,
        status: "pending"
      }
    });
  } catch (error) {
    const msg = error instanceof Error ? error.message : "unknown error";
    res.status(500).json({ ok: false, message: msg });
  }
});

// ---------- Device pull API for ESP ----------
app.get("/api/device/pull", (req, res) => {
  const deviceId = String(req.query.device_id || "");
  const apiKey = String(req.query.api_key || "");
  if (!deviceId || !apiKey) return res.status(400).send("Missing device_id/api_key");

  void (async () => {
    const client = await pool.connect();
    try {
      const auth = await client.query(
        "SELECT id FROM devices WHERE id = $1 AND api_key = $2 AND enabled = TRUE",
        [deviceId, apiKey]
      );
      if (!auth.rowCount) {
        client.release();
        return res.status(401).send("Unauthorized device");
      }

      await client.query("BEGIN");
      const cmdRes = await client.query(
        `
        SELECT id, type, from_name, text_payload, image_data, created_at
        FROM commands
        WHERE device_id = $1 AND status = 'pending'
        ORDER BY id ASC
        LIMIT 1
        FOR UPDATE SKIP LOCKED
        `,
        [deviceId]
      );

      if (!cmdRes.rowCount) {
        await client.query("COMMIT");
        client.release();
        return res.status(204).end();
      }

      const cmd = cmdRes.rows[0] as {
        id: number;
        type: "text" | "image";
        from_name: string;
        text_payload: string | null;
        image_data: Buffer | null;
        created_at: number;
      };

      await client.query(
        "UPDATE commands SET status = 'delivered', delivered_at = $2 WHERE id = $1",
        [cmd.id, Math.floor(Date.now() / 1000)]
      );
      await client.query("COMMIT");
      client.release();

      res.setHeader("Cache-Control", "no-store");
      res.setHeader("X-Cmd-Id", String(cmd.id));
      res.setHeader("X-Cmd-Type", cmd.type);
      res.setHeader("X-From", cmd.from_name || "Web");
      res.setHeader("X-Timestamp", String(cmd.created_at));

      if (cmd.type === "text") {
        return res.type("text/plain; charset=utf-8").send(cmd.text_payload || "");
      }

      if (!cmd.image_data) {
        return res.status(410).send("Missing image payload");
      }
      return res.type("application/octet-stream").send(cmd.image_data);
    } catch (err) {
      try {
        await client.query("ROLLBACK");
      } catch {
        // ignore rollback error
      }
      client.release();
      throw err;
    }
  })().catch((err) => {
    res.status(500).send(`DB error: ${err.message}`);
  });
});

// ---------- Static client ----------
const clientDistDir = path.resolve(process.cwd(), "dist/client");
if (fs.existsSync(clientDistDir)) {
  app.use(express.static(clientDistDir));
  app.get("*", (req: Request, res: Response) => {
    if (req.path.startsWith("/api/")) return res.status(404).end();
    return res.sendFile(path.join(clientDistDir, "index.html"));
  });
}

void (async () => {
  await initDb();
  app.listen(config.backendPort, () => {
    console.log(`paper-web backend listening on :${config.backendPort}`);
    console.log("storage: PostgreSQL");
  });
})().catch((err) => {
  console.error("Fatal startup error:", err);
  process.exit(1);
});
