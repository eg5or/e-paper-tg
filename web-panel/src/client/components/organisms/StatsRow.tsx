import { Box, ButtonBase, Typography } from "@mui/material";
import { PageCard } from "@/components/atoms/PageCard";
import type { Stats } from "@/types";

interface StatsRowProps {
  stats: Stats;
  onPendingClick?: () => void;
}

export function StatsRow({ stats, onPendingClick }: StatsRowProps) {
  const items = [
    { label: "Устройств", value: stats.devices },
    { label: "В очереди", value: stats.pending, clickable: true },
    { label: "Доставлено", value: stats.delivered }
  ];
  return (
    <Box display="grid" gridTemplateColumns={{ xs: "1fr", md: "repeat(3, 1fr)" }} gap={2} mb={2}>
      {items.map((item) => (
        <Box key={item.label}>
          {item.clickable ? (
            <ButtonBase
              onClick={() => onPendingClick?.()}
              sx={{ display: "block", width: "100%", borderRadius: 2, textAlign: "left" }}
            >
              <PageCard sx={{ width: "100%" }}>
                <Typography variant="body2" color="text.secondary">
                  {item.label}
                </Typography>
                <Typography variant="h4">{item.value}</Typography>
                <Typography variant="caption" color="primary.main">
                  Открыть очередь
                </Typography>
              </PageCard>
            </ButtonBase>
          ) : (
            <PageCard>
              <Typography variant="body2" color="text.secondary">
                {item.label}
              </Typography>
              <Typography variant="h4">{item.value}</Typography>
            </PageCard>
          )}
        </Box>
      ))}
    </Box>
  );
}
