import { FormControl, InputLabel, MenuItem, Select } from "@mui/material";
import type { Device } from "@/types";

interface DeviceSelectProps {
  devices: Device[];
  value: string;
  onChange: (value: string) => void;
  label?: string;
}

export function DeviceSelect({ devices, value, onChange, label = "Устройство" }: DeviceSelectProps) {
  return (
    <FormControl fullWidth>
      <InputLabel>{label}</InputLabel>
      <Select
        value={value}
        label={label}
        onChange={(e) => onChange(e.target.value)}
      >
        {devices
          .filter((d) => d.enabled)
          .map((device) => (
            <MenuItem key={device.id} value={device.id}>
              {device.name} ({device.id})
            </MenuItem>
          ))}
      </Select>
    </FormControl>
  );
}
