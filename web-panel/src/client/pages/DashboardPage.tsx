import { Box } from "@mui/material";
import { useQuery, useQueryClient } from "@tanstack/react-query";
import { useNavigate } from "react-router-dom";
import { api } from "@/api/client";
import { StatsRow } from "@/components/organisms/StatsRow";
import { TextSenderCard } from "@/components/organisms/TextSenderCard";
import { ImageSenderCard } from "@/components/organisms/ImageSenderCard";

export function DashboardPage() {
  const navigate = useNavigate();
  const queryClient = useQueryClient();
  const devicesQuery = useQuery({ queryKey: ["devices"], queryFn: api.devices });
  const statsQuery = useQuery({ queryKey: ["stats"], queryFn: api.stats });

  const refreshAfterSend = async () => {
    await queryClient.invalidateQueries({ queryKey: ["stats"] });
    await queryClient.invalidateQueries({ queryKey: ["queue"] });
    await queryClient.invalidateQueries({ queryKey: ["delivered"] });
  };

  const devices = devicesQuery.data || [];
  const stats = statsQuery.data || { pending: 0, delivered: 0, devices: 0 };

  return (
    <>
      <StatsRow
        stats={stats}
        onPendingClick={() => {
          navigate("/queue");
        }}
        onDeliveredClick={() => {
          navigate("/delivered");
        }}
      />
      <Box display="grid" gridTemplateColumns={{ xs: "1fr", md: "1fr 1fr" }} gap={2}>
        <Box>
          <TextSenderCard
            devices={devices}
            onSend={async (payload) => {
              await api.sendText(payload);
              await refreshAfterSend();
            }}
          />
        </Box>
        <Box>
          <ImageSenderCard
            devices={devices}
            onPreview={api.previewImage}
            onSend={async (payload) => {
              await api.sendImage(payload);
              await refreshAfterSend();
            }}
          />
        </Box>
      </Box>
    </>
  );
}
