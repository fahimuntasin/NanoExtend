import { Link } from "react-router-dom";
import { ArrowLeft, Moon, Sun } from "lucide-react";
import { ClaudeArt, SignalMark } from "./ClaudeArt";
import { MadeBy } from "./MadeBy";
import { useTheme } from "./theme";
import UsbSetup from "./UsbSetup";

export default function DashboardPage() {
  const { theme, toggleTheme } = useTheme();

  return (
    <div className="dash-app">
      <ClaudeArt />
      <header className="dash-app-bar">
        <div className="dash-app-brand">
          <SignalMark size={32} />
          <div>
            <strong>NanoExtend</strong>
            <span>USB Dashboard · 1.0.4</span>
          </div>
        </div>
        <div className="dash-app-actions">
          <button
            aria-label="Toggle theme"
            className="button white-btn theme-toggle"
            onClick={toggleTheme}
            type="button"
          >
            {theme === "light" ? <Moon size={16} /> : <Sun size={16} />}
            {theme === "light" ? "Dark" : "Light"}
          </button>
          <Link className="button white-btn" to="/">
            <ArrowLeft size={16} />
            Landing
          </Link>
          <Link className="button white-btn" to="/docs">
            Docs
          </Link>
          <Link className="button white-btn" to="/changelog">
            Changelog
          </Link>
        </div>
      </header>

      <main className="dash-app-main">
        <UsbSetup fullPage />
      </main>

      <footer className="dash-app-footer">
        <MadeBy />
      </footer>
    </div>
  );
}
