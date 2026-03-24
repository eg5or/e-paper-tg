import type { Device, QueueItem, Stats } from "@/types";

async function parseJson<T>(res: Response): Promise<T> {
  const json = (await res.json()) as T & { ok?: boolean; message?: string };
  if (!res.ok || (typeof (json as { ok?: boolean }).ok === "boolean" && !(json as { ok?: boolean }).ok)) {
    throw new Error((json as { message?: string }).message || "Request failed");
  }
  return json;
}

export const api = {
  async me(): Promise<{ isAuthed: boolean }> {
    const res = await fetch("/api/auth/me", { credentials: "include" });
    return parseJson<{ ok: true; isAuthed: boolean }>(res);
  },

  async login(username: string, password: string): Promise<void> {
    const res = await fetch("/api/auth/login", {
      method: "POST",
      credentials: "include",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ username, password })
    });
    await parseJson<{ ok: true }>(res);
  },

  async logout(): Promise<void> {
    const res = await fetch("/api/auth/logout", {
      method: "POST",
      credentials: "include"
    });
    await parseJson<{ ok: true }>(res);
  },

  async devices(): Promise<Device[]> {
    const res = await fetch("/api/panel/devices", { credentials: "include" });
    const json = await parseJson<{ ok: true; devices: Device[] }>(res);
    return json.devices;
  },

  async upsertDevice(payload: {
    id: string;
    name: string;
    apiKey: string;
    enabled: boolean;
  }): Promise<void> {
    const res = await fetch("/api/panel/devices", {
      method: "POST",
      credentials: "include",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload)
    });
    await parseJson<{ ok: true }>(res);
  },

  async deleteDevice(id: string): Promise<void> {
    const res = await fetch(`/api/panel/devices/${encodeURIComponent(id)}`, {
      method: "DELETE",
      credentials: "include"
    });
    await parseJson<{ ok: true }>(res);
  },

  async stats(): Promise<Stats> {
    const res = await fetch("/api/panel/stats", { credentials: "include" });
    const json = await parseJson<{ ok: true; stats: Stats }>(res);
    return json.stats;
  },

  async queue(): Promise<QueueItem[]> {
    const res = await fetch("/api/panel/queue", { credentials: "include" });
    const json = await parseJson<{ ok: true; queue: QueueItem[] }>(res);
    return json.queue;
  },

  async sendText(payload: { deviceId: string; fromName: string; text: string }): Promise<void> {
    const res = await fetch("/api/panel/send/text", {
      method: "POST",
      credentials: "include",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload)
    });
    await parseJson<{ ok: true }>(res);
  },

  async sendImage(payload: { deviceId: string; fromName: string; image: File }): Promise<void> {
    const form = new FormData();
    form.append("deviceId", payload.deviceId);
    form.append("fromName", payload.fromName);
    form.append("image", payload.image);
    const res = await fetch("/api/panel/send/image", {
      method: "POST",
      credentials: "include",
      body: form
    });
    await parseJson<{ ok: true }>(res);
  },

  async previewImage(image: File): Promise<{ blob: Blob; width: number; height: number }> {
    const form = new FormData();
    form.append("image", image);
    const res = await fetch("/api/panel/image/preview", {
      method: "POST",
      credentials: "include",
      body: form
    });
    if (!res.ok) {
      let message = "Request failed";
      try {
        const json = (await res.json()) as { message?: string };
        message = json.message || message;
      } catch {
        // keep default
      }
      throw new Error(message);
    }

    const width = Number(res.headers.get("X-Image-Width") || 0);
    const height = Number(res.headers.get("X-Image-Height") || 0);
    const blob = await res.blob();
    return { blob, width, height };
  }
};
