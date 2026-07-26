import { StrictMode, useEffect } from "react";
import { createRoot } from "react-dom/client";
import { HashRouter, Navigate, Route, Routes, useLocation } from "react-router-dom";
import "./index.css";
import LandingPage from "./App.tsx";
import ChangelogPage from "./ChangelogPage.tsx";
import DashboardPage from "./DashboardPage.tsx";
import DocsPage from "./DocsPage.tsx";
import { ThemeProvider } from "./theme.tsx";

function LegacyHashRedirect() {
  const location = useLocation();
  useEffect(() => {
    const raw = window.location.hash.replace(/^#/, "");
    if (raw === "usb-setup" || raw.startsWith("usb-setup")) {
      window.location.hash = "#/dashboard";
    }
  }, [location]);
  return null;
}

createRoot(document.getElementById("root")!).render(
  <StrictMode>
    <ThemeProvider>
      <HashRouter>
        <LegacyHashRedirect />
        <Routes>
          <Route path="/" element={<LandingPage />} />
          <Route path="/dashboard" element={<DashboardPage />} />
          <Route path="/docs" element={<DocsPage />} />
          <Route path="/changelog" element={<ChangelogPage />} />
          <Route path="*" element={<Navigate replace to="/" />} />
        </Routes>
      </HashRouter>
    </ThemeProvider>
  </StrictMode>,
);
