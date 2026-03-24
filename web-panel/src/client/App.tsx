import { api } from "@/api/client";
import { AppTopBar } from "@/components/organisms/AppTopBar";
import { DashboardPage } from "@/pages/DashboardPage";
import { DeliveredPage } from "@/pages/DeliveredPage";
import { LoginPage } from "@/pages/LoginPage";
import { QueuePage } from "@/pages/QueuePage";
import { SettingsPage } from "@/pages/SettingsPage";
import { Box, CircularProgress, Container, CssBaseline } from "@mui/material";
import { useQuery, useQueryClient } from "@tanstack/react-query";
import { Navigate, Route, Routes, useNavigate } from "react-router-dom";

function ProtectedApp() {
  const navigate = useNavigate();
  const queryClient = useQueryClient();
  return (
    <>
      <AppTopBar
        onLogout={async () => {
          await api.logout();
          queryClient.setQueryData(["auth", "me"], { ok: true, isAuthed: false });
          navigate("/login");
        }}
      />
      <Container maxWidth="lg" sx={{ py: 3 }}>
        <Routes>
          <Route path="/" element={<DashboardPage />} />
          <Route path="/queue" element={<QueuePage />} />
          <Route path="/delivered" element={<DeliveredPage />} />
          <Route path="/settings" element={<SettingsPage />} />
          <Route path="*" element={<Navigate to="/" replace />} />
        </Routes>
      </Container>
    </>
  );
}

export default function App() {
  const meQuery = useQuery({
    queryKey: ["auth", "me"],
    queryFn: api.me
  });

  if (meQuery.isLoading) {
    return (
      <Box sx={{ minHeight: "100vh", display: "grid", placeItems: "center" }}>
        <CircularProgress />
      </Box>
    );
  }

  const isAuthed = meQuery.data?.isAuthed ?? false;

  return (
    <>
      <CssBaseline />
      <Routes>
        <Route path="/login" element={isAuthed ? <Navigate to="/" replace /> : <LoginPage />} />
        <Route path="/*" element={isAuthed ? <ProtectedApp /> : <Navigate to="/login" replace />} />
      </Routes>
    </>
  );
}
