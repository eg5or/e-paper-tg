import { Box, Container, Typography } from "@mui/material";
import { useQueryClient } from "@tanstack/react-query";
import { useNavigate } from "react-router-dom";
import { PageCard } from "@/components/atoms/PageCard";
import { LoginForm } from "@/components/molecules/LoginForm";
import { api } from "@/api/client";

export function LoginPage() {
  const navigate = useNavigate();
  const queryClient = useQueryClient();

  return (
    <Container maxWidth="sm" sx={{ py: 10 }}>
      <PageCard>
        <Box mb={2}>
          <Typography variant="h5">Paper Web Login</Typography>
          <Typography variant="body2" color="text.secondary">
            Войдите, чтобы отправлять сообщения и настраивать устройства.
          </Typography>
        </Box>
        <LoginForm
          onSubmit={async (username, password) => {
            await api.login(username, password);
            queryClient.setQueryData(["auth", "me"], { ok: true, isAuthed: true });
            navigate("/");
          }}
        />
      </PageCard>
    </Container>
  );
}
