import { Link } from "react-router-dom";
import { ArrowLeft, Moon, Sun } from "lucide-react";
import { ClaudeArt, SignalMark } from "./ClaudeArt";
import { MadeBy } from "./MadeBy";
import { useTheme } from "./theme";

const sections = [
  {
    title: "Quick start",
    body: [
      "Flash NanoExtend with the browser installer (Chrome/Edge + USB).",
      "Open the USB Dashboard — no phone SoftAP required.",
      "Pick CH340 / USB Serial (ignore ttyS* motherboard ports).",
      "Scan Wi-Fi, connect upstream, manage settings and logs over the cable.",
    ],
  },
  {
    title: "USB dashboard",
    body: [
      "Firmware 1.0.3+ speaks an NE> JSON line protocol at 115200 baud.",
      "Tabs mirror SoftAP: Home, Scan, Clients, Settings, System, Logs.",
      "On connect, the onboard LED plays a double-pulse celebrate pattern.",
      "OTA binary upload still uses SoftAP or the installer.",
    ],
  },
  {
    title: "SoftAP path",
    body: [
      "Join SSID NanoExtend (default password changeme123).",
      "Open http://192.168.4.1 for the on-device SPA.",
      "Change the AP password immediately after first boot.",
    ],
  },
  {
    title: "Security notes",
    body: [
      "USB serial admin equals physical access — treat like SoftAP admin.",
      "Admin HTTP APIs stay SoftAP-subnet bound by default.",
      "Do not leave an unlocked PC attached on untrusted networks.",
    ],
  },
];

export default function DocsPage() {
  const { theme, toggleTheme } = useTheme();

  return (
    <div className="doc-app">
      <ClaudeArt />
      <header className="dash-app-bar">
        <Link className="dash-app-brand" to="/">
          <SignalMark size={32} />
          <div>
            <strong>NanoExtend Docs</strong>
            <span>Guides for humans and agents</span>
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
        <h1>Documentation</h1>
        <p className="doc-lede">
          Everything you need to flash, configure, and operate NanoExtend from a
          desktop — including the USB-first dashboard path.
        </p>
        {sections.map((section) => (
          <section className="doc-card" key={section.title}>
            <h2>{section.title}</h2>
            <ul>
              {section.body.map((line) => (
                <li key={line}>{line}</li>
              ))}
            </ul>
          </section>
        ))}
        <p className="doc-links">
          Full markdown lives in the repo:{" "}
          <a href="https://github.com/fahimuntasin/NanoExtend/tree/main/docs">
            /docs
          </a>
          .
        </p>
      </main>
      <footer className="dash-app-footer">
        <MadeBy />
      </footer>
    </div>
  );
}
