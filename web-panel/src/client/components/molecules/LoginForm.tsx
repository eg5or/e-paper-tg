import { Alert, Button, Stack, TextField } from "@mui/material";
import { useState } from "react";

interface LoginFormProps {
  onSubmit: (username: string, password: string) => Promise<void>;
}

export function LoginForm({ onSubmit }: LoginFormProps) {
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [error, setError] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);

  return (
    <Stack
      spacing={2}
      component="form"
      onSubmit={async (e) => {
        e.preventDefault();
        setError(null);
        setLoading(true);
        try {
          await onSubmit(username, password);
        } catch (err) {
          setError(err instanceof Error ? err.message : "Ошибка входа");
        } finally {
          setLoading(false);
        }
      }}
    >
      {error ? <Alert severity="error">{error}</Alert> : null}
      <TextField
        label="Логин"
        value={username}
        onChange={(e) => setUsername(e.target.value)}
        required
      />
      <TextField
        label="Пароль"
        type="password"
        value={password}
        onChange={(e) => setPassword(e.target.value)}
        required
      />
      <Button type="submit" variant="contained" disabled={loading}>
        {loading ? "Входим..." : "Войти"}
      </Button>
    </Stack>
  );
}
