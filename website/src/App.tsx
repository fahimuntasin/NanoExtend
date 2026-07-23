import {
  Activity,
  ArrowRight,
  BookOpen,
  Box,
  Check,
  ChevronDown,
  CloudDownload,
  Code2,
  Cpu,
  GitFork,
  Globe2,
  LockKeyhole,
  MemoryStick,
  Network,
  Radio,
  RefreshCw,
  Router,
  ShieldCheck,
  Sparkles,
  TerminalSquare,
  Users,
  Wifi,
  Zap,
} from "lucide-react";
import "./App.css";
import Installer from "./Installer";
import Logo from "./Logo";

const repoUrl = "https://github.com/fahimuntasin/NanoExtend";

const features = [
  [
    Network,
    "Real NAT routing",
    "lwIP NAPT, DHCP, DNS forwarding, and automatic route restoration.",
  ],
  [
    Radio,
    "Self-healing Wi-Fi",
    "Backoff, BSSID recovery, health probes, and a SoftAP that stays available.",
  ],
  [
    Activity,
    "Live telemetry",
    "WebSocket updates for signal, memory, clients, uptime, logs, and errors.",
  ],
  [
    CloudDownload,
    "Safe updates",
    "Dual OTA slots, SHA-256 verification, stable-boot confirmation, and rollback.",
  ],
  [
    ShieldCheck,
    "Secure by default",
    "SoftAP-bound admin API, session + CSRF checks, validation, and rate limiting.",
  ],
  [
    Sparkles,
    "A better embedded UX",
    "A responsive local dashboard with no CDN and no internet dependency.",
  ],
] as const;

function App() {
  return (
    <div className="site-shell">
      <header className="nav-wrap">
        <a className="brand-link" href="#top" aria-label="NanoExtend home">
          <Logo size={30} animated />
          <span>NanoExtend</span>
        </a>
        <nav aria-label="Main navigation">
          <a href="#features">Features</a>
          <a href="#installer">Install</a>
          <a href="#docs">Docs</a>
          <a href="#roadmap">Roadmap</a>
        </nav>
        <a
          className="github-link"
          href={repoUrl}
          rel="noreferrer"
          target="_blank"
        >
          <GitFork size={17} aria-hidden="true" />
          GitHub
        </a>
      </header>

      <main id="main">
        <section className="hero-section" id="top">
          <div className="hero-copy">
            <div className="release-pill">
              <span />
              NanoExtend 1.0 · Open source
              <ArrowRight size={14} aria-hidden="true" />
            </div>
            <h1>
              Tiny ESP32.
              <br />
              <span>Big Network.</span>
            </h1>
            <p>
              Turn one ESP32 into a secure, self-healing Wi-Fi travel router
              with a dashboard that feels at home in 2026.
            </p>
            <div className="hero-actions">
              <a className="button primary" href="#installer">
                Install NanoExtend <ArrowRight size={17} aria-hidden="true" />
              </a>
              <a className="button secondary" href="#docs">
                Read the docs
              </a>
            </div>
            <div className="hero-proof">
              <span>
                <Check size={14} /> MIT licensed
              </span>
              <span>
                <Check size={14} /> No cloud required
              </span>
              <span>
                <Check size={14} /> 4 MB ready
              </span>
            </div>
          </div>

          <div className="hero-visual" aria-label="NanoExtend network preview">
            <div className="orbit orbit-one" />
            <div className="orbit orbit-two" />
            <div className="device-card">
              <div className="device-card-top">
                <Logo size={42} animated />
                <div>
                  <strong>NanoExtend</strong>
                  <span>192.168.4.1</span>
                </div>
                <i>Online</i>
              </div>
              <div className="network-line">
                <span className="network-node">
                  <Globe2 size={18} />
                </span>
                <span className="pulse-line" />
                <span className="network-node main-node">
                  <Router size={20} />
                </span>
                <span className="pulse-line reverse" />
                <span className="network-node">
                  <Wifi size={18} />
                </span>
              </div>
              <div className="metric-grid">
                <div>
                  <small>Signal</small>
                  <strong>−42 dBm</strong>
                  <div className="mini-chart signal-chart">
                    <i />
                  </div>
                </div>
                <div>
                  <small>Free heap</small>
                  <strong>171 KB</strong>
                  <div className="mini-chart heap-chart">
                    <i />
                  </div>
                </div>
                <div>
                  <small>Clients</small>
                  <strong>2</strong>
                  <span className="avatars">
                    <b>A</b>
                    <b>M</b>
                  </span>
                </div>
                <div>
                  <small>Uptime</small>
                  <strong>18d 4h</strong>
                  <span className="stable">Stable</span>
                </div>
              </div>
            </div>
          </div>
        </section>

        <section className="trusted-strip" aria-label="Project qualities">
          {[
            ["AP + STA", Network],
            ["lwIP NAPT", Router],
            ["Realtime", Activity],
            ["OTA rollback", RefreshCw],
            ["Local first", LockKeyhole],
          ].map(([label, Icon]) => (
            <span key={label as string}>
              <Icon size={16} />
              {label as string}
            </span>
          ))}
        </section>

        <section className="section" id="features">
          <div className="section-heading">
            <span className="eyebrow">The complete router</span>
            <h2>Small hardware. Serious capabilities.</h2>
            <p>
              Every layer is designed for predictable operation on an ESP32
              DevKit V1.
            </p>
          </div>
          <div className="feature-grid">
            {features.map(([Icon, title, text]) => (
              <article className="feature-card" key={title}>
                <div className="feature-icon">
                  <Icon size={20} />
                </div>
                <h3>{title}</h3>
                <p>{text}</p>
              </article>
            ))}
          </div>
        </section>

        <section className="section dashboard-section">
          <div className="section-heading left">
            <span className="eyebrow">Dashboard preview</span>
            <h2>Everything important, at a glance.</h2>
            <p>
              Connect upstream Wi-Fi, inspect clients, restore settings, and
              update firmware without a terminal.
            </p>
          </div>
          <div className="dashboard-preview">
            <aside>
              <div className="preview-brand">
                <Logo size={25} />
                NanoExtend
              </div>
              {[
                ["Overview", Box],
                ["Network", Wifi],
                ["Clients", Users],
                ["System", Cpu],
                ["Updates", RefreshCw],
              ].map(([label, Icon], index) => (
                <span
                  className={index === 0 ? "selected" : ""}
                  key={label as string}
                >
                  <Icon size={15} />
                  {label as string}
                </span>
              ))}
            </aside>
            <div className="preview-content">
              <div className="preview-title">
                <div>
                  <small>OVERVIEW</small>
                  <strong>Good afternoon</strong>
                </div>
                <span>All systems operational</span>
              </div>
              <div className="preview-stats">
                {[
                  ["Internet", "Online", Globe2],
                  ["Signal", "Excellent", Radio],
                  ["Memory", "171 KB free", MemoryStick],
                  ["Clients", "2 connected", Users],
                ].map(([label, value, Icon]) => (
                  <div key={label as string}>
                    <Icon size={17} />
                    <small>{label as string}</small>
                    <strong>{value as string}</strong>
                  </div>
                ))}
              </div>
              <div className="preview-graph">
                <div>
                  <strong>Network quality</strong>
                  <small>Last 60 seconds</small>
                </div>
                <svg
                  preserveAspectRatio="none"
                  viewBox="0 0 720 170"
                  aria-hidden="true"
                >
                  <defs>
                    <linearGradient id="lineFill" x1="0" x2="0" y1="0" y2="1">
                      <stop offset="0" stopColor="white" stopOpacity=".16" />
                      <stop offset="1" stopColor="white" stopOpacity="0" />
                    </linearGradient>
                  </defs>
                  <path
                    d="M0 134 C60 124 84 92 132 106 S210 140 258 92 S330 48 386 76 S470 122 516 70 S604 30 720 48 V170 H0Z"
                    fill="url(#lineFill)"
                  />
                  <path
                    d="M0 134 C60 124 84 92 132 106 S210 140 258 92 S330 48 386 76 S470 122 516 70 S604 30 720 48"
                    fill="none"
                    stroke="white"
                    strokeWidth="2"
                  />
                </svg>
              </div>
            </div>
          </div>
        </section>

        <section className="section installer-section" id="installer">
          <div className="section-heading">
            <span className="eyebrow">Five-minute setup</span>
            <h2>From box to router, in one tab.</h2>
            <p>
              Chip detection, firmware download, checksum verification,
              flashing, and restart.
            </p>
          </div>
          <Installer />
        </section>

        <section className="section ota-section">
          <div className="ota-copy">
            <span className="eyebrow">Update center</span>
            <h2>Updates that earn your trust.</h2>
            <p>
              Every image is checked before activation. The previous working
              firmware remains available until the new build proves it can stay
              healthy.
            </p>
            <ul>
              <li>
                <Check size={15} /> SHA-256 image verification
              </li>
              <li>
                <Check size={15} /> Dual-slot rollback strategy
              </li>
              <li>
                <Check size={15} /> Release notes and version history
              </li>
              <li>
                <Check size={15} /> GitHub Releases and custom servers
              </li>
            </ul>
          </div>
          <div className="update-card">
            <div className="update-card-head">
              <div className="update-icon">
                <CloudDownload size={23} />
              </div>
              <div>
                <strong>NanoExtend 1.1.0</strong>
                <span>Ready to install</span>
              </div>
              <b>Stable</b>
            </div>
            <p>Improved network recovery and a faster dashboard.</p>
            <div className="update-meta">
              <span>1.21 MB</span>
              <span>SHA-256 verified</span>
              <span>2 min</span>
            </div>
            <button className="button primary" type="button">
              Install update <ArrowRight size={16} />
            </button>
          </div>
        </section>

        <section className="section" id="docs">
          <div className="section-heading">
            <span className="eyebrow">Documentation</span>
            <h2>Clear paths for every kind of builder.</h2>
          </div>
          <div className="docs-grid">
            {[
              [
                Zap,
                "Getting started",
                "Flash your first board and connect in five minutes.",
                `${repoUrl}/blob/main/docs/getting-started.md`,
              ],
              [
                TerminalSquare,
                "Developer guide",
                "Build, test, debug, and understand the project.",
                `${repoUrl}/blob/main/docs/developer-guide.md`,
              ],
              [
                Code2,
                "API reference",
                "Versioned REST and WebSocket contracts with OpenAPI.",
                "/api/openapi.yaml",
              ],
              [
                ShieldCheck,
                "Security",
                "Threat model, OTA trust, reporting, and hardening.",
                `${repoUrl}/blob/main/SECURITY.md`,
              ],
            ].map(([Icon, title, text, href]) => (
              <a
                className="doc-card"
                href={href as string}
                key={title as string}
              >
                <Icon size={20} />
                <h3>{title as string}</h3>
                <p>{text as string}</p>
                <span>
                  Explore <ArrowRight size={14} />
                </span>
              </a>
            ))}
          </div>
        </section>

        <section className="section roadmap-section" id="roadmap">
          <div className="section-heading left">
            <span className="eyebrow">Public roadmap</span>
            <h2>Built in the open. Improved together.</h2>
          </div>
          <div className="roadmap">
            {[
              [
                "Now",
                "1.0",
                "Reliable NAT, captive portal, live dashboard, local OTA",
              ],
              [
                "Next",
                "1.1",
                "Remote manifests, signed releases, bandwidth metrics",
              ],
              [
                "Later",
                "2.0",
                "Board profiles, SDK, and optional mesh experiments",
              ],
            ].map(([phase, version, detail], index) => (
              <article key={phase}>
                <span className={index === 0 ? "live" : ""}>{phase}</span>
                <strong>{version}</strong>
                <p>{detail}</p>
              </article>
            ))}
          </div>
        </section>

        <section className="section faq-section">
          <div className="section-heading">
            <span className="eyebrow">FAQ</span>
            <h2>Answers before you plug in.</h2>
          </div>
          <div className="faq-list">
            {[
              [
                "Which ESP32 board do I need?",
                "ESP32 DevKit V1 with 4 MB flash and no PSRAM.",
              ],
              [
                "Does it need a cloud account?",
                "No. Routing, configuration, telemetry, and local OTA work directly on the device.",
              ],
              [
                "How many clients can connect?",
                "One or two clients, prioritizing reliability over maximum throughput.",
              ],
              [
                "Can a failed OTA brick the router?",
                "Dual OTA slots keep the previous image until stable-boot verification passes.",
              ],
            ].map(([question, answer]) => (
              <details key={question}>
                <summary>
                  {question}
                  <ChevronDown size={18} />
                </summary>
                <p>{answer}</p>
              </details>
            ))}
          </div>
        </section>

        <section className="section contributors">
          <div>
            <BookOpen size={21} />
            <span>Open documentation</span>
          </div>
          <div>
            <GitFork size={21} />
            <span>Community maintained</span>
          </div>
          <div>
            <LockKeyhole size={21} />
            <span>Public security policy</span>
          </div>
          <a className="button secondary" href={repoUrl}>
            Become a contributor <ArrowRight size={16} />
          </a>
        </section>
      </main>

      <footer>
        <div className="footer-brand">
          <Logo size={34} animated />
          <div>
            <strong>NanoExtend</strong>
            <span>Tiny ESP32. Big Network.</span>
          </div>
        </div>
        <div className="footer-links">
          <div>
            <strong>Project</strong>
            <a href="#features">Features</a>
            <a href="#roadmap">Roadmap</a>
            <a href={`${repoUrl}/releases`}>Releases</a>
          </div>
          <div>
            <strong>Resources</strong>
            <a href="#docs">Documentation</a>
            <a href="/api/openapi.yaml">OpenAPI</a>
            <a href={`${repoUrl}/discussions`}>Discussions</a>
          </div>
          <div>
            <strong>Community</strong>
            <a href={`${repoUrl}/blob/main/CONTRIBUTING.md`}>Contributing</a>
            <a href={`${repoUrl}/blob/main/SECURITY.md`}>Security</a>
            <a href={`${repoUrl}/blob/main/LICENSE`}>MIT License</a>
          </div>
        </div>
        <div className="footer-bottom">
          <span>© 2026 NanoExtend contributors.</span>
          <span>Designed and built in the open.</span>
        </div>
      </footer>
    </div>
  );
}

export default App;
