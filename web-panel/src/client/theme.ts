import { createTheme } from "@mui/material/styles";

export const theme = createTheme({
  palette: {
    mode: "dark",
    primary: { main: "#4f8cff" },
    background: {
      default: "#0f172a",
      paper: "#1e293b"
    }
  },
  shape: { borderRadius: 12 },
  typography: {
    fontFamily: "Inter, Arial, sans-serif"
  }
});
