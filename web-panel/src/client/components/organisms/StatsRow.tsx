import { Box, ButtonBase, Typography } from "@mui/material";
import { PageCard } from "@/components/atoms/PageCard";
import type { Stats } from "@/types";

interface StatsRowProps {
  stats: Stats;
  onPendingClick?: () => void;
  onDeliveredClick?: () => void;
}

export function StatsRow({ stats, onPendingClick, onDeliveredClick }: StatsRowProps) {
  const items = [
    { label: "Устройств", value: stats.devices },
    { label: "В очереди", value: stats.pending, clickable: true, hint: "Открыть очередь", onClick: onPendingClick },
    {
      label: "Доставлено",
      value: stats.delivered,
      clickable: true,
      hint: "Открыть отправленные",
      onClick: onDeliveredClick
    }
  ];
  return (
    <Box display="grid" gridTemplateColumns={{ xs: "1fr", md: "repeat(3, 1fr)" }} gap={2} mb={2}>
      {items.map((item) => (
        <Box key={item.label}>
          {item.clickable ? (
            <ButtonBase
              onClick={() => item.onClick?.()}
              sx={{ display: "block", width: "100%", borderRadius: 2, textAlign: "left" }}
            >
              <PageCard sx={{ width: "100%" }}>
                <Typography variant="body2" color="text.secondary">
                  {item.label}
                </Typography>
                <Typography variant="h4">{item.value}</Typography>
                <Typography variant="caption" color="primary.main">
                  {item.hint}
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
