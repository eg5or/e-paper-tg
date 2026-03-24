import { Alert, Button, FormControlLabel, Stack, Switch, TextField } from "@mui/material";
import { useState } from "react";
import { PageCard } from "@/components/atoms/PageCard";
import { SectionHeader } from "@/components/atoms/SectionHeader";

interface DeviceFormCardProps {
  onSave: (payload: { id: string; name: string; apiKey: string; enabled: boolean }) => Promise<void>;
}

export function DeviceFormCard({ onSave }: DeviceFormCardProps) {
  const genKey = () => {
    const bytes = new Uint8Array(16);
    globalThis.crypto.getRandomValues(bytes);
    return Array.from(bytes)
      .map((b) => b.toString(16).padStart(2, "0"))
      .join("");
  };

  const [id, setId] = useState("");
  const [name, setName] = useState("");
  const [apiKey, setApiKey] = useState(genKey());
  const [enabled, setEnabled] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [success, setSuccess] = useState<string | null>(null);

  return (
    <PageCard>
      <SectionHeader title="Добавить / обновить устройство" />
      <Stack spacing={2}>
        {error ? <Alert severity="error">{error}</Alert> : null}
        {success ? <Alert severity="success">{success}</Alert> : null}
        <TextField label="Device ID" value={id} onChange={(e) => setId(e.target.value)} required />
        <TextField label="Название" value={name} onChange={(e) => setName(e.target.value)} required />
        <TextField label="API Key" value={apiKey} onChange={(e) => setApiKey(e.target.value)} required />
        <FormControlLabel
          control={<Switch checked={enabled} onChange={(e) => setEnabled(e.target.checked)} />}
          label="Enabled"
        />
        <Button
          variant="contained"
          onClick={async () => {
            setError(null);
            setSuccess(null);
            try {
              await onSave({ id, name, apiKey, enabled });
              setSuccess("Устройство сохранено.");
              setId("");
              setName("");
              setApiKey(genKey());
              setEnabled(true);
            } catch (err) {
              setError(err instanceof Error ? err.message : "Ошибка сохранения");
            }
          }}
        >
          Сохранить
        </Button>
      </Stack>
    </PageCard>
  );
}
