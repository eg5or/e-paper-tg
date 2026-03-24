export interface Device {
  id: string;
  name: string;
  apiKey: string;
  enabled: boolean;
  createdAt: number;
}

export interface Stats {
  pending: number;
  delivered: number;
  devices: number;
}

export interface QueueItem {
  id: number;
  deviceId: string;
  type: "text" | "image";
  fromName: string;
  textPayload: string | null;
  imageWidth: number | null;
  imageHeight: number | null;
  hasPreview: boolean;
  createdAt: number;
  deliveredAt?: number | null;
}
