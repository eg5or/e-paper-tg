import { Alert, Button, Stack, TextField } from "@mui/material";
import { useState } from "react";
import { PageCard } from "@/components/atoms/PageCard";
import { SectionHeader } from "@/components/atoms/SectionHeader";
import { DeviceSelect } from "@/components/molecules/DeviceSelect";
import type { Device } from "@/types";

interface TextSenderCardProps {
  devices: Device[];
  onSend: (payload: { deviceId: string; fromName: string; text: string }) => Promise<void>;
}

export function TextSenderCard({ devices, onSend }: TextSenderCardProps) {
  const [deviceId, setDeviceId] = useState(devices.find((d) => d.enabled)?.id || "");
  const [fromName, setFromName] = useState("Web");
  const [text, setText] = useState("");
  const [error, setError] = useState<string | null>(null);
  const [success, setSuccess] = useState<string | null>(null);

  return (
    <PageCard>
      <SectionHeader title="Отправить текст" subtitle="Сообщение попадет в очередь выбранного устройства." />
      <Stack spacing={2}>
        {error ? <Alert severity="error">{error}</Alert> : null}
        {success ? <Alert severity="success">{success}</Alert> : null}
        <DeviceSelect devices={devices} value={deviceId} onChange={setDeviceId} />
        <TextField label="Имя отправителя" value={fromName} onChange={(e) => setFromName(e.target.value)} />
        <TextField label="Текст" value={text} onChange={(e) => setText(e.target.value)} multiline minRows={6} />
        <Button
          variant="contained"
          onClick={async () => {
            setError(null);
            setSuccess(null);
            try {
              await onSend({ deviceId, fromName, text });
              setSuccess("Текст отправлен в очередь.");
              setText("");
            } catch (err) {
              setError(err instanceof Error ? err.message : "Ошибка отправки");
            }
          }}
        >
          Отправить текст
        </Button>
      </Stack>
    </PageCard>
  );
}
