import { AppBar, Box, Button, Toolbar, Typography } from "@mui/material";
import { Link as RouterLink } from "react-router-dom";

interface AppTopBarProps {
  onLogout: () => Promise<void>;
}

export function AppTopBar({ onLogout }: AppTopBarProps) {
  return (
    <AppBar position="static" elevation={0}>
      <Toolbar>
        <Typography variant="h6" sx={{ flexGrow: 1 }}>
          Paper Web Panel
        </Typography>
        <Button color="inherit" component={RouterLink} to="/">
          Отправка
        </Button>
        <Button color="inherit" component={RouterLink} to="/queue">
          Очередь
        </Button>
        <Button color="inherit" component={RouterLink} to="/settings">
          Настройки
        </Button>
        <Box ml={1}>
          <Button color="inherit" onClick={() => void onLogout()}>
            Выйти
          </Button>
        </Box>
      </Toolbar>
    </AppBar>
  );
}
