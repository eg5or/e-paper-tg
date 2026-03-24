export type CommandType = "text" | "image";
export type CommandStatus = "pending" | "delivered";

export interface Device {
  id: string;
  name: string;
  apiKey: string;
  enabled: boolean;
  createdAt: number;
}

export interface Command {
  id: number;
  deviceId: string;
  type: CommandType;
  fromName: string;
  text?: string;
  rawPath?: string;
  imageWidth?: number;
  imageHeight?: number;
  createdAt: number;
  deliveredAt?: number;
  status: CommandStatus;
}

export interface DBState {
  devices: Device[];
  commands: Command[];
  nextCommandId: number;
}
