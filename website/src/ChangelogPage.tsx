import { Link } from "react-router-dom";
import { ArrowLeft, Moon, Sun } from "lucide-react";
import { ClaudeArt, SignalMark } from "./ClaudeArt";
import { MadeBy } from "./MadeBy";
import { useTheme } from "./theme";

const releases = [
  {
    version: "1.0.4",
    date: "2026-07-26",
    items: [
      "Dedicated full-page USB dashboard with theme switcher.",
      "Claude-inspired light theme, live SVG atmosphere, white action buttons.",
      "Onboard LED celebrate pattern on USB hello / connect.",
      "In-site Docs and Changelog pages; Made by fahimuntasin.com credit.",
    ],
  },
  {
    version: "1.0.3",
    date: "2026-07-26",
    items: [
      "Full USB SoftAP-style tabs: Home, Scan, Clients, Settings, System, Logs.",
      "SerialAdmin: clients, logs, health, factory_reset.",
    ],
  },
  {
    version: "1.0.2",
    date: "2026-07-26",
    items: [
      "USB serial NE> JSON admin for phone-free Wi-Fi setup.",
      "Product photography showcase on README and landing.",
    ],
  },
  {
    version: "1.0.1",
    date: "2026-07-23",
    items: [
      "GitHub Releases as default OTA update source.",
      "GitHub Actions Node 24-compatible updates.",
    ],
  },
  {
    version: "1.0.0",
    date: "2026-07-23",
    items: [
      "Initial production release: SoftAP+STA NAT router, dashboard, OTA, docs.",
    ],
  },
];

export default function ChangelogPage() {
  const { theme, toggleTheme } = useTheme();

  return (
    <div className="doc-app">
      <ClaudeArt />
      <header className="dash-app-bar">
        <Link className="dash-app-brand" to="/">
          <SignalMark size={32} />
          <div>
            <strong>Changelog</strong>
            <span>What shipped, and why</span>
          </div>
        </Link>
        <div className="dash-app-actions">
          <button
            className="button white-btn theme-toggle"
            onClick={toggleTheme}
            type="button"
          >
            {theme === "light" ? <Moon size={16} /> : <Sun size={16} />}
            {theme === "light" ? "Dark" : "Light"}
          </button>
          <Link className="button white-btn" to="/dashboard">
            Dashboard
          </Link>
          <Link className="button white-btn" to="/">
            <ArrowLeft size={16} /> Home
          </Link>
        </div>
      </header>

      <main className="doc-main">
        <h1>Changelog</h1>
        <p className="doc-lede">
          Keep a Changelog style notes for NanoExtend firmware and website.
        </p>
        {releases.map((release) => (
          <section className="doc-card" key={release.version}>
            <div className="changelog-head">
              <h2>v{release.version}</h2>
              <time>{release.date}</time>
            </div>
            <ul>
              {release.items.map((item) => (
                <li key={item}>{item}</li>
              ))}
            </ul>
          </section>
        ))}
      </main>
      <footer className="dash-app-footer">
        <MadeBy />
      </footer>
    </div>
  );
}
