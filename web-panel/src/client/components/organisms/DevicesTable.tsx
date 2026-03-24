import ContentCopyIcon from "@mui/icons-material/ContentCopy";
import VisibilityIcon from "@mui/icons-material/Visibility";
import {
  Button,
  Chip,
  Dialog,
  DialogActions,
  DialogContent,
  DialogTitle,
  IconButton,
  Stack,
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableRow,
  TextField,
  Tooltip
} from "@mui/material";
import { useState } from "react";
import { PageCard } from "@/components/atoms/PageCard";
import { SectionHeader } from "@/components/atoms/SectionHeader";
import type { Device } from "@/types";

interface DevicesTableProps {
  devices: Device[];
  onDelete: (id: string) => Promise<void>;
}

export function DevicesTable({ devices, onDelete }: DevicesTableProps) {
  const [selectedKey, setSelectedKey] = useState<string | null>(null);
  const [copiedId, setCopiedId] = useState<string | null>(null);

  const handleCopy = async (deviceId: string, apiKey: string) => {
    await navigator.clipboard.writeText(apiKey);
    setCopiedId(deviceId);
    setTimeout(() => setCopiedId(null), 1200);
  };

  return (
    <>
      <PageCard>
        <SectionHeader title="Список устройств" />
        <Table size="small">
          <TableHead>
            <TableRow>
              <TableCell>ID</TableCell>
              <TableCell>Name</TableCell>
              <TableCell>API Key</TableCell>
              <TableCell>Status</TableCell>
              <TableCell />
            </TableRow>
          </TableHead>
          <TableBody>
            {devices.map((d) => (
              <TableRow key={d.id}>
                <TableCell>{d.id}</TableCell>
                <TableCell>{d.name}</TableCell>
                <TableCell>
                  <Stack direction="row" spacing={1} alignItems="center">
                    <span>{d.apiKey.slice(0, 6)}...{d.apiKey.slice(-4)}</span>
                    <Tooltip title="Показать полностью">
                      <IconButton size="small" onClick={() => setSelectedKey(d.apiKey)}>
                        <VisibilityIcon fontSize="small" />
                      </IconButton>
                    </Tooltip>
                    <Tooltip title={copiedId === d.id ? "Скопировано" : "Скопировать"}>
                      <IconButton size="small" onClick={() => void handleCopy(d.id, d.apiKey)}>
                        <ContentCopyIcon fontSize="small" />
                      </IconButton>
                    </Tooltip>
                  </Stack>
                </TableCell>
                <TableCell>
                  <Chip size="small" label={d.enabled ? "enabled" : "disabled"} color={d.enabled ? "success" : "default"} />
                </TableCell>
                <TableCell align="right">
                  <Button color="error" onClick={() => void onDelete(d.id)}>
                    Удалить
                  </Button>
                </TableCell>
              </TableRow>
            ))}
          </TableBody>
        </Table>
      </PageCard>

      <Dialog open={Boolean(selectedKey)} onClose={() => setSelectedKey(null)} maxWidth="sm" fullWidth>
        <DialogTitle>API Key устройства</DialogTitle>
        <DialogContent>
          <TextField value={selectedKey || ""} fullWidth multiline minRows={2} />
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setSelectedKey(null)}>Закрыть</Button>
          <Button
            variant="contained"
            onClick={async () => {
              if (!selectedKey) return;
              await navigator.clipboard.writeText(selectedKey);
            }}
          >
            Скопировать
          </Button>
        </DialogActions>
      </Dialog>
    </>
  );
}
