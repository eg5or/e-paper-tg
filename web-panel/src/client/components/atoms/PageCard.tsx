import { Card, CardContent, type CardProps } from "@mui/material";
import type { PropsWithChildren } from "react";

export function PageCard({ children, ...props }: PropsWithChildren<CardProps>) {
  return (
    <Card elevation={0} {...props}>
      <CardContent>{children}</CardContent>
    </Card>
  );
}
