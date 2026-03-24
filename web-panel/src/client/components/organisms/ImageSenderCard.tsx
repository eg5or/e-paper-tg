import { Alert, Box, Button, Stack, TextField, Typography } from "@mui/material";
import { useEffect, useState } from "react";
import { PageCard } from "@/components/atoms/PageCard";
import { SectionHeader } from "@/components/atoms/SectionHeader";
import { DeviceSelect } from "@/components/molecules/DeviceSelect";
import type { Device } from "@/types";

interface ImageSenderCardProps {
  devices: Device[];
  onSend: (payload: { deviceId: string; fromName: string; image: File }) => Promise<void>;
  onPreview: (image: File) => Promise<{ blob: Blob; width: number; height: number }>;
}

export function ImageSenderCard({ devices, onSend, onPreview }: ImageSenderCardProps) {
  const [deviceId, setDeviceId] = useState(devices.find((d) => d.enabled)?.id || "");
  const [fromName, setFromName] = useState("Web");
  const [file, setFile] = useState<File | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [success, setSuccess] = useState<string | null>(null);
  const [previewUrl, setPreviewUrl] = useState<string | null>(null);
  const [previewSize, setPreviewSize] = useState<{ width: number; height: number } | null>(null);
  const [previewReady, setPreviewReady] = useState(false);
  const [isConverting, setIsConverting] = useState(false);
  const [isSending, setIsSending] = useState(false);

  useEffect(() => {
    return () => {
      if (previewUrl) URL.revokeObjectURL(previewUrl);
    };
  }, [previewUrl]);

  return (
    <PageCard>
      <SectionHeader title="Отправить изображение" subtitle="Сначала конвертируйте и проверьте превью, затем отправляйте." />
      <Stack spacing={2}>
        {error ? <Alert severity="error">{error}</Alert> : null}
        {success ? <Alert severity="success">{success}</Alert> : null}
        <DeviceSelect devices={devices} value={deviceId} onChange={setDeviceId} />
        <TextField label="Имя отправителя" value={fromName} onChange={(e) => setFromName(e.target.value)} />
        <Button variant="outlined" component="label">
          {file ? `Файл: ${file.name}` : "Выбрать файл"}
          <input
            hidden
            type="file"
            accept="image/*"
            onChange={(e) => {
              setFile(e.target.files?.[0] || null);
              setPreviewReady(false);
              setPreviewSize(null);
              if (previewUrl) {
                URL.revokeObjectURL(previewUrl);
                setPreviewUrl(null);
              }
            }}
          />
        </Button>

        <Button
          variant="outlined"
          disabled={!file || isConverting}
          onClick={async () => {
            setError(null);
            setSuccess(null);
            if (!file) {
              setError("Выберите файл");
              return;
            }
            try {
              setIsConverting(true);
              const preview = await onPreview(file);
              if (previewUrl) URL.revokeObjectURL(previewUrl);
              const newUrl = URL.createObjectURL(preview.blob);
              setPreviewUrl(newUrl);
              setPreviewSize({ width: preview.width, height: preview.height });
              setPreviewReady(true);
              setSuccess("Превью готово. Проверьте результат перед отправкой.");
            } catch (err) {
              setError(err instanceof Error ? err.message : "Ошибка конвертации");
            } finally {
              setIsConverting(false);
            }
          }}
        >
          {isConverting ? "Преобразование..." : "Преобразовать в монохром"}
        </Button>

        {previewUrl ? (
          <Box>
            <Typography variant="body2" color="text.secondary" mb={1}>
              Превью для устройства{previewSize ? ` (${previewSize.width}x${previewSize.height})` : ""}:
            </Typography>
            <Box
              component="img"
              src={previewUrl}
              alt="Monochrome preview"
              sx={{ width: "100%", maxHeight: 260, objectFit: "contain", borderRadius: 1, border: "1px solid", borderColor: "divider" }}
            />
          </Box>
        ) : null}

        <Button
          variant="contained"
          disabled={!previewReady || !file || isSending}
          onClick={async () => {
            setError(null);
            setSuccess(null);
            if (!file) {
              setError("Выберите файл");
              return;
            }
            if (!previewReady) {
              setError("Сначала выполните преобразование и проверьте превью");
              return;
            }
            try {
              setIsSending(true);
              await onSend({ deviceId, fromName, image: file });
              setSuccess("Изображение отправлено в очередь.");
              setFile(null);
              setPreviewReady(false);
              setPreviewSize(null);
              if (previewUrl) {
                URL.revokeObjectURL(previewUrl);
                setPreviewUrl(null);
              }
            } catch (err) {
              setError(err instanceof Error ? err.message : "Ошибка отправки");
            } finally {
              setIsSending(false);
            }
          }}
        >
          {isSending ? "Отправка..." : "Отправить изображение"}
        </Button>
      </Stack>
    </PageCard>
  );
}
