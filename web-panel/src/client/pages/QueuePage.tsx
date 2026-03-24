import {
  Alert,
  Box,
  Chip,
  Dialog,
  DialogContent,
  FormControl,
  InputLabel,
  MenuItem,
  Select,
  Stack,
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableRow,
  Typography
} from "@mui/material";
import { useQuery } from "@tanstack/react-query";
import { useMemo, useState } from "react";
import { api } from "@/api/client";
import type { QueueItem } from "@/types";
import { PageCard } from "@/components/atoms/PageCard";

const REFRESH_STORAGE_KEY = "paper.queue.refreshSeconds";
const DEFAULT_REFRESH_SECONDS = 20;
const refreshOptions = [3, 5, 10, 15, 20, 30, 45, 60];

function formatUnixTime(unixTs: number): string {
  return new Date(unixTs * 1000).toLocaleString();
}

function payloadPreview(row: QueueItem): string {
  if (row.type === "text") {
    if (!row.textPayload) return "";
    return row.textPayload.length > 120 ? `${row.textPayload.slice(0, 120)}...` : row.textPayload;
  }
  const w = row.imageWidth ?? "?";
  const h = row.imageHeight ?? "?";
  return `RAW image ${w}x${h}`;
}

export function QueuePage() {
  const [refreshSeconds, setRefreshSeconds] = useState<number>(() => {
    if (typeof window === "undefined") return DEFAULT_REFRESH_SECONDS;
    const saved = Number(window.localStorage.getItem(REFRESH_STORAGE_KEY) || DEFAULT_REFRESH_SECONDS);
    if (!Number.isFinite(saved) || saved < 3 || saved > 60) return DEFAULT_REFRESH_SECONDS;
    return saved;
  });
  const [previewDialog, setPreviewDialog] = useState<{ open: boolean; src: string; title: string }>({
    open: false,
    src: "",
    title: ""
  });

  const refreshMs = useMemo(() => refreshSeconds * 1000, [refreshSeconds]);

  const queueQuery = useQuery({
    queryKey: ["queue"],
    queryFn: api.queue,
    refetchInterval: refreshMs
  });

  const rows = queueQuery.data || [];

  return (
    <Stack spacing={2}>
      <PageCard>
        <Stack direction={{ xs: "column", md: "row" }} spacing={2} alignItems={{ xs: "stretch", md: "center" }}>
          <Box sx={{ flexGrow: 1 }}>
            <Typography variant="h6">Очередь команд</Typography>
            <Typography variant="body2" color="text.secondary">
              Показаны команды со статусом pending.
            </Typography>
          </Box>
          <FormControl size="small" sx={{ minWidth: 220 }}>
            <InputLabel id="queue-refresh-label">Обновление</InputLabel>
            <Select
              labelId="queue-refresh-label"
              label="Обновление"
              value={refreshSeconds}
              onChange={(e) => {
                const next = Number(e.target.value);
                setRefreshSeconds(next);
                if (typeof window !== "undefined") {
                  window.localStorage.setItem(REFRESH_STORAGE_KEY, String(next));
                }
              }}
            >
              {refreshOptions.map((sec) => (
                <MenuItem key={sec} value={sec}>
                  Каждые {sec} сек
                </MenuItem>
              ))}
            </Select>
          </FormControl>
        </Stack>
      </PageCard>

      {queueQuery.isError ? (
        <Alert severity="error">{queueQuery.error instanceof Error ? queueQuery.error.message : "Ошибка загрузки"}</Alert>
      ) : null}

      <PageCard>
        <Table size="small">
          <TableHead>
            <TableRow>
              <TableCell>ID</TableCell>
              <TableCell>Device</TableCell>
              <TableCell>Тип</TableCell>
              <TableCell>От</TableCell>
              <TableCell>Создано</TableCell>
              <TableCell>Содержимое</TableCell>
              <TableCell>Превью</TableCell>
            </TableRow>
          </TableHead>
          <TableBody>
            {rows.map((row) => (
              <TableRow key={row.id}>
                <TableCell>{row.id}</TableCell>
                <TableCell>{row.deviceId}</TableCell>
                <TableCell>
                  <Chip size="small" color={row.type === "text" ? "primary" : "secondary"} label={row.type} />
                </TableCell>
                <TableCell>{row.fromName}</TableCell>
                <TableCell>{formatUnixTime(row.createdAt)}</TableCell>
                <TableCell>{payloadPreview(row)}</TableCell>
                <TableCell>
                  {row.type === "image" && row.hasPreview ? (
                    <img
                      src={`/api/panel/queue/${row.id}/preview`}
                      alt={`Preview ${row.id}`}
                      onClick={() =>
                        setPreviewDialog({
                          open: true,
                          src: `/api/panel/queue/${row.id}/preview`,
                          title: `Команда #${row.id} (${row.imageWidth ?? "?"}x${row.imageHeight ?? "?"})`
                        })
                      }
                      style={{
                        cursor: "zoom-in",
                        width: 96,
                        height: 72,
                        objectFit: "contain",
                        borderRadius: 6,
                        border: "1px solid rgba(255,255,255,0.12)"
                      }}
                    />
                  ) : (
                    "-"
                  )}
                </TableCell>
              </TableRow>
            ))}
          </TableBody>
        </Table>
      </PageCard>

      <Dialog
        open={previewDialog.open}
        onClose={() => setPreviewDialog({ open: false, src: "", title: "" })}
        maxWidth="md"
        fullWidth
      >
        <DialogContent>
          <Stack spacing={1}>
            <Typography variant="subtitle1">{previewDialog.title}</Typography>
            <Box
              component="img"
              src={previewDialog.src}
              alt={previewDialog.title}
              sx={{
                width: "100%",
                maxHeight: "80vh",
                objectFit: "contain",
                borderRadius: 1,
                border: "1px solid",
                borderColor: "divider"
              }}
            />
          </Stack>
        </DialogContent>
      </Dialog>
    </Stack>
  );
}
