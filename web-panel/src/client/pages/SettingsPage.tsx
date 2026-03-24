import { Box } from "@mui/material";
import { useQuery, useQueryClient } from "@tanstack/react-query";
import { api } from "@/api/client";
import { DeviceFormCard } from "@/components/organisms/DeviceFormCard";
import { DevicesTable } from "@/components/organisms/DevicesTable";

export function SettingsPage() {
  const queryClient = useQueryClient();
  const devicesQuery = useQuery({ queryKey: ["devices"], queryFn: api.devices });

  const refresh = async () => {
    await queryClient.invalidateQueries({ queryKey: ["devices"] });
    await queryClient.invalidateQueries({ queryKey: ["stats"] });
  };

  const devices = devicesQuery.data || [];

  return (
    <Box display="grid" gridTemplateColumns={{ xs: "1fr", md: "5fr 7fr" }} gap={2}>
      <Box>
        <DeviceFormCard
          onSave={async (payload) => {
            await api.upsertDevice(payload);
            await refresh();
          }}
        />
      </Box>
      <Box>
        <DevicesTable
          devices={devices}
          onDelete={async (id) => {
            await api.deleteDevice(id);
            await refresh();
          }}
        />
      </Box>
    </Box>
  );
}
