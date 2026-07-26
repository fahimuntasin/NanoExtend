import { useCallback, useEffect, useRef, useState, type FormEvent } from "react";
import {
  LoaderCircle,
  PlugZap,
  RefreshCw,
  Unplug,
  Usb,
  Wifi,
} from "lucide-react";
import { requestEspSerialPort, SERIAL_PORT_HINT } from "./serialPorts";

type ConnState = "idle" | "connecting" | "ready" | "error";
type TabId = "home" | "scan" | "clients" | "settings" | "system" | "logs";

type ScanNetwork = {
  ssid: string;
  rssi: number;
  channel: number;
  encryption: number;
};

type WifiStatus = {
  state?: string;
  apSsid?: string;
  apIp?: string;
  apClients?: number;
  staConnected?: boolean;
  staSsid?: string;
  rssi?: number;
  staIp?: string;
  gateway?: string;
  dns?: string;
  nat?: boolean;
  sharing?: boolean;
  autoReconnectPaused?: boolean;
};

type SystemStatus = {
  firmware?: string;
  uptimeSec?: number;
  freeHeap?: number;
  minFreeHeap?: number;
  cpuMhz?: number;
  sdk?: string;
  resetReason?: string;
  reconnectCount?: number;
  lastError?: string;
  flashSize?: number;
  sketchSize?: number;
  freeSketch?: number;
  crashPending?: boolean;
  crashText?: string;
  health?: {
    state?: string;
    detail?: string;
    dns?: boolean;
    icmp?: boolean;
    http?: boolean;
    https?: boolean;
  };
};

type StatusResult = {
  apIp?: string;
  staIp?: string;
  wifi?: WifiStatus;
  system?: SystemStatus;
};

type ClientRow = { mac?: string; rssi?: number; ip?: string; hostname?: string };

type SettingsForm = {
  apSsid: string;
  apPass: string;
  deviceName: string;
  hostname: string;
};

type NeReply = {
  v: number;
  id: number;
  ok: boolean;
  error?: string;
  result?: Record<string, unknown>;
};

const tabs: { id: TabId; label: string }[] = [
  { id: "home", label: "Home" },
  { id: "scan", label: "Scan" },
  { id: "clients", label: "Clients" },
  { id: "settings", label: "Settings" },
  { id: "system", label: "System" },
  { id: "logs", label: "Logs" },
];

function formatUptime(sec?: number): string {
  if (sec == null) return "—";
  const d = Math.floor(sec / 86400);
  const h = Math.floor((sec % 86400) / 3600);
  const m = Math.floor((sec % 3600) / 60);
  if (d > 0) return `${d}d ${h}h`;
  if (h > 0) return `${h}h ${m}m`;
  return `${m}m ${sec % 60}s`;
}

function encryptionLabel(enc: number): string {
  return enc === 0 ? "Open" : "Secured";
}

function UsbSetup({ fullPage = false }: { fullPage?: boolean }) {
  const [conn, setConn] = useState<ConnState>("idle");
  const [message, setMessage] = useState(
    "Plug in the ESP32. This USB dashboard mirrors the SoftAP UI — no phone Wi‑Fi needed.",
  );
  const [fw, setFw] = useState("");
  const [tab, setTab] = useState<TabId>("home");
  const [status, setStatus] = useState<StatusResult | null>(null);
  const [networks, setNetworks] = useState<ScanNetwork[]>([]);
  const [clients, setClients] = useState<ClientRow[]>([]);
  const [settings, setSettings] = useState<SettingsForm>({
    apSsid: "",
    apPass: "",
    deviceName: "",
    hostname: "",
  });
  const [logs, setLogs] = useState("");
  const [scanning, setScanning] = useState(false);
  const [busy, setBusy] = useState(false);
  const [modalSsid, setModalSsid] = useState<string | null>(null);
  const [modalPass, setModalPass] = useState("");
  const [modalStatus, setModalStatus] = useState("");

  const portRef = useRef<SerialPort | null>(null);
  const readerRef = useRef<ReadableStreamDefaultReader<Uint8Array> | null>(null);
  const writerRef = useRef<WritableStreamDefaultWriter<Uint8Array> | null>(null);
  const bufferRef = useRef("");
  const decoderRef = useRef(new TextDecoder());
  const pendingRef = useRef(
    new Map<
      number,
      {
        resolve: (value: NeReply) => void;
        reject: (reason?: unknown) => void;
        timer: number;
      }
    >(),
  );
  const nextIdRef = useRef(1);
  const pollRef = useRef<number | null>(null);
  const supported = "serial" in navigator;

  const cleanupPort = useCallback(async () => {
    if (pollRef.current) {
      window.clearInterval(pollRef.current);
      pollRef.current = null;
    }
    for (const pending of pendingRef.current.values()) {
      window.clearTimeout(pending.timer);
      pending.reject(new Error("Disconnected"));
    }
    pendingRef.current.clear();
    try {
      await readerRef.current?.cancel();
    } catch {
      /* ignore */
    }
    readerRef.current = null;
    try {
      writerRef.current?.releaseLock();
    } catch {
      /* ignore */
    }
    writerRef.current = null;
    const port = portRef.current;
    portRef.current = null;
    if (port) {
      try {
        await port.close();
      } catch {
        /* ignore */
      }
    }
  }, []);

  const handleLine = useCallback((line: string) => {
    if (!line.startsWith("NE{")) return;
    try {
      const reply = JSON.parse(line.slice(2)) as NeReply;
      const pending = pendingRef.current.get(reply.id);
      if (!pending) return;
      window.clearTimeout(pending.timer);
      pendingRef.current.delete(reply.id);
      pending.resolve(reply);
    } catch {
      /* ignore malformed */
    }
  }, []);

  const readLoop = useCallback(
    async (reader: ReadableStreamDefaultReader<Uint8Array>) => {
      try {
        while (true) {
          const { value, done } = await reader.read();
          if (done) break;
          bufferRef.current += decoderRef.current.decode(value, { stream: true });
          let newline = bufferRef.current.indexOf("\n");
          while (newline >= 0) {
            const raw = bufferRef.current.slice(0, newline).replace(/\r$/, "");
            bufferRef.current = bufferRef.current.slice(newline + 1);
            if (raw) handleLine(raw);
            newline = bufferRef.current.indexOf("\n");
          }
        }
      } catch {
        /* closed */
      }
    },
    [handleLine],
  );

  const request = useCallback(
    async (cmd: string, extra: Record<string, unknown> = {}, timeoutMs = 8000) => {
      const writer = writerRef.current;
      if (!writer) throw new Error("USB serial is not connected.");
      const id = nextIdRef.current++;
      const line = `NE>${JSON.stringify({ v: 1, id, cmd, ...extra })}\n`;
      const reply = await new Promise<NeReply>((resolve, reject) => {
        const timer = window.setTimeout(() => {
          pendingRef.current.delete(id);
          reject(new Error(`Timeout waiting for ${cmd}`));
        }, timeoutMs);
        pendingRef.current.set(id, { resolve, reject, timer });
        writer.write(new TextEncoder().encode(line)).catch((error) => {
          window.clearTimeout(timer);
          pendingRef.current.delete(id);
          reject(error);
        });
      });
      if (!reply.ok) throw new Error(reply.error || `${cmd} failed`);
      return reply.result ?? {};
    },
    [],
  );

  const refreshStatus = useCallback(async () => {
    const result = (await request("status")) as StatusResult;
    setStatus(result);
    return result;
  }, [request]);

  const refreshScan = useCallback(
    async (refresh = true) => {
      setScanning(true);
      try {
        const result = (await request("scan", { refresh }, 12000)) as {
          networks?: ScanNetwork[];
          inProgress?: boolean;
        };
        setNetworks(result.networks ?? []);
        if (result.inProgress) {
          window.setTimeout(() => {
            void refreshScan(false);
          }, 1200);
        } else {
          setScanning(false);
        }
      } catch (error) {
        setScanning(false);
        throw error;
      }
    },
    [request],
  );

  const refreshClients = useCallback(async () => {
    const result = (await request("clients")) as { clients?: ClientRow[] };
    setClients(result.clients ?? []);
  }, [request]);

  const refreshSettings = useCallback(async () => {
    const result = (await request("settings_get")) as Partial<SettingsForm>;
    setSettings({
      apSsid: result.apSsid ?? "",
      apPass: result.apPass ?? "",
      deviceName: result.deviceName ?? "",
      hostname: result.hostname ?? "",
    });
  }, [request]);

  const refreshLogs = useCallback(async () => {
    const result = (await request("logs", {}, 10000)) as {
      text?: string;
      truncated?: boolean;
    };
    setLogs(
      `${result.truncated ? "…truncated…\n" : ""}${result.text ?? "(empty)"}`,
    );
  }, [request]);

  const connectUsb = async () => {
    if (!supported) {
      setConn("error");
      setMessage("Web Serial is not supported in this browser.");
      return;
    }
    setConn("connecting");
    setMessage(SERIAL_PORT_HINT);
    setBusy(true);
    try {
      await cleanupPort();
      const port = await requestEspSerialPort();
      await port.open({ baudRate: 115200 });
      portRef.current = port;
      bufferRef.current = "";
      decoderRef.current = new TextDecoder();
      if (!port.readable || !port.writable) {
        throw new Error("Serial port streams are unavailable.");
      }
      const reader = (port.readable as ReadableStream<Uint8Array>).getReader();
      readerRef.current = reader;
      void readLoop(reader);
      writerRef.current = (
        port.writable as WritableStream<Uint8Array>
      ).getWriter();

      // CH340 toggles DTR/RTS on open and reboots the ESP — wait for SerialAdmin.
      setMessage("USB open — waiting for NanoExtend to boot…");
      await new Promise((r) => window.setTimeout(r, 4000));
      bufferRef.current = "";
      // Nudge the UART and drop any bootloader noise still in flight.
      await writerRef.current.write(new TextEncoder().encode("\n\n"));
      await new Promise((r) => window.setTimeout(r, 200));
      bufferRef.current = "";

      let hello: Record<string, unknown> | null = null;
      for (let attempt = 0; attempt < 20; attempt++) {
        setMessage(`Waiting for firmware reply… (${attempt + 1}/20)`);
        try {
          hello = await request("hello", {}, 3000);
          break;
        } catch {
          await new Promise((r) => window.setTimeout(r, 600));
        }
      }
      if (!hello) {
        throw new Error(
          "No reply from NanoExtend. Unplug/replug USB, close other serial monitors, hard-refresh this page, then pick CH340/USB Serial (not ttyS*).",
        );
      }

      setFw(String(hello.fw ?? ""));
      setConn("ready");
      setMessage(
        hello.led
          ? "Connected — watch the board LED celebrate, then configure Wi‑Fi."
          : "USB dashboard live — same controls as SoftAP, over the cable.",
      );
      try {
        await refreshStatus();
      } catch {
        /* status can retry via poll */
      }
      try {
        await refreshSettings();
      } catch {
        /* settings tab can reload */
      }
      try {
        await refreshScan(true);
      } catch {
        setMessage(
          "USB dashboard live. Wi-Fi scan is still warming up — open the Scan tab and refresh.",
        );
      }

      pollRef.current = window.setInterval(() => {
        void refreshStatus().catch(() => undefined);
      }, 2500);
    } catch (error) {
      await cleanupPort();
      setConn("error");
      setMessage(error instanceof Error ? error.message : "USB connection failed.");
    } finally {
      setBusy(false);
    }
  };

  const disconnectUsb = async () => {
    setBusy(true);
    await cleanupPort();
    setConn("idle");
    setStatus(null);
    setNetworks([]);
    setClients([]);
    setLogs("");
    setFw("");
    setMessage("Disconnected. Reconnect USB to open the dashboard again.");
    setBusy(false);
  };

  const connectWifi = async () => {
    if (!modalSsid) return;
    setBusy(true);
    setModalStatus(`Connecting to ${modalSsid}…`);
    try {
      await request(
        "connect",
        { ssid: modalSsid, password: modalPass },
        25000,
      );
      setModalStatus("Accepted. Waiting for STA…");
      for (let i = 0; i < 12; i++) {
        await new Promise((r) => window.setTimeout(r, 1000));
        const st = await refreshStatus();
        if (st.staIp) {
          setModalStatus(`Connected · ${st.staIp}`);
          break;
        }
      }
      window.setTimeout(() => {
        setModalSsid(null);
        setModalPass("");
        setModalStatus("");
      }, 800);
    } catch (error) {
      setModalStatus(error instanceof Error ? error.message : "Connect failed.");
    } finally {
      setBusy(false);
    }
  };

  const saveSettings = async (event: FormEvent) => {
    event.preventDefault();
    setBusy(true);
    try {
      await request("settings_set", settings);
      setMessage("Settings saved.");
      await refreshStatus();
    } catch (error) {
      setMessage(error instanceof Error ? error.message : "Save failed.");
    } finally {
      setBusy(false);
    }
  };

  useEffect(() => {
    if (conn !== "ready") return;
    if (tab === "clients") void refreshClients().catch(() => undefined);
    if (tab === "logs") void refreshLogs().catch(() => undefined);
    if (tab === "settings") void refreshSettings().catch(() => undefined);
    if (tab === "scan") void refreshScan(false).catch(() => undefined);
  }, [
    tab,
    conn,
    refreshClients,
    refreshLogs,
    refreshSettings,
    refreshScan,
  ]);

  useEffect(() => {
    return () => {
      void cleanupPort();
    };
  }, [cleanupPort]);

  const wifi = status?.wifi;
  const sys = status?.system;

    return (
    <div className={`usb-dash ${fullPage ? "usb-dash-full" : ""}`}>
      {conn !== "ready" ? (
        <div className="usb-dash-gate">
          <div className="usb-dash-gate-head">
            <div>
              <span className="eyebrow">USB dashboard</span>
              <h3>
                {fullPage
                  ? "Connect once. Control everything."
                  : "Same SoftAP UI. Over the cable."}
              </h3>
            </div>
            <div className="device-pill">
              <Usb size={15} aria-hidden="true" />
              Waiting for USB
            </div>
          </div>
          <p className={conn === "error" ? "installer-error" : "installer-message"}>
            {message}
          </p>
          <aside className="usb-port-hint" aria-label="Serial port tip">
            <strong>Which port?</strong>
            <span>
              Pick <em>USB Serial</em> / <em>CH340</em> / <em>CP210x</em>. Skip{" "}
              <em>ttyS0–ttyS15</em> — those belong to the PC, not the ESP32.
              On connect the onboard LED plays a double-pulse celebrate.
            </span>
          </aside>
          <button
            className="button primary installer-button"
            disabled={!supported || busy || conn === "connecting"}
            onClick={() => void connectUsb()}
            type="button"
          >
            {conn === "connecting" ? (
              <LoaderCircle className="spin" size={17} aria-hidden="true" />
            ) : (
              <PlugZap size={17} aria-hidden="true" />
            )}
            Connect USB dashboard
          </button>
        </div>
      ) : (
        <>
          <header className="usb-dash-top">
            <div>
              <div className="usb-dash-brand">NanoExtend</div>
              <div className="usb-dash-sub">
                USB · {fw || status?.system?.firmware || "ready"} ·{" "}
                {wifi?.state ?? "—"}
                {wifi?.autoReconnectPaused ? " · reconnect paused" : ""}
              </div>
            </div>
            <div className="usb-dash-top-actions">
              <span className="usb-dash-pill">
                {status?.staIp || status?.apIp || "192.168.4.1"}
              </span>
              <button
                className="button secondary"
                disabled={busy}
                onClick={() => void disconnectUsb()}
                type="button"
              >
                <Unplug size={15} aria-hidden="true" />
                Disconnect
              </button>
            </div>
          </header>

          <nav className="usb-dash-tabs" aria-label="USB dashboard tabs">
            {tabs.map((item) => (
              <button
                className={tab === item.id ? "active" : ""}
                key={item.id}
                onClick={() => setTab(item.id)}
                type="button"
              >
                {item.label}
              </button>
            ))}
          </nav>

          <p className="usb-dash-note">{message}</p>

          {tab === "home" && (
            <section className="usb-dash-panel">
              <div className="usb-dash-grid">
                <div className="usb-dash-card">
                  <small>Connection</small>
                  <strong>{wifi?.state ?? "—"}</strong>
                </div>
                <div className="usb-dash-card">
                  <small>Upstream SSID</small>
                  <strong>{wifi?.staSsid || "Not connected"}</strong>
                </div>
                <div className="usb-dash-card">
                  <small>RSSI</small>
                  <strong>
                    {wifi?.staConnected ? `${wifi.rssi} dBm` : "—"}
                  </strong>
                </div>
                <div className="usb-dash-card">
                  <small>STA IP</small>
                  <strong>{status?.staIp || "—"}</strong>
                </div>
                <div className="usb-dash-card">
                  <small>Gateway</small>
                  <strong>{wifi?.gateway || "—"}</strong>
                </div>
                <div className="usb-dash-card">
                  <small>DNS</small>
                  <strong>{wifi?.dns || "—"}</strong>
                </div>
                <div className="usb-dash-card">
                  <small>NAT / Sharing</small>
                  <strong>
                    {wifi?.nat ? "NAT on" : "NAT off"}
                    {wifi?.sharing ? " · sharing" : ""}
                  </strong>
                </div>
                <div className="usb-dash-card">
                  <small>Internet</small>
                  <strong>{sys?.health?.state ?? "—"}</strong>
                </div>
                <div className="usb-dash-card">
                  <small>AP clients</small>
                  <strong>{wifi?.apClients ?? 0}</strong>
                </div>
                <div className="usb-dash-card">
                  <small>Uptime</small>
                  <strong>{formatUptime(sys?.uptimeSec)}</strong>
                </div>
                <div className="usb-dash-card">
                  <small>Free heap</small>
                  <strong>
                    {sys?.freeHeap != null
                      ? `${Math.round(sys.freeHeap / 1024)} KB`
                      : "—"}
                  </strong>
                </div>
                <div className="usb-dash-card">
                  <small>Reconnects</small>
                  <strong>{sys?.reconnectCount ?? 0}</strong>
                </div>
                <div className="usb-dash-card wide">
                  <small>Last error</small>
                  <strong>{sys?.lastError || "none"}</strong>
                </div>
              </div>
              <div className="usb-dash-row">
                <button
                  className="button secondary"
                  disabled={busy || scanning}
                  onClick={() => {
                    setTab("scan");
                    void refreshScan(true);
                  }}
                  type="button"
                >
                  <Wifi size={15} /> Scan
                </button>
                <button
                  className="button secondary"
                  disabled={busy}
                  onClick={() =>
                    void request("disconnect")
                      .then(() => refreshStatus())
                      .then(() =>
                        setMessage(
                          "STA stopped — auto-reconnect paused. Rescan, then connect with the correct password.",
                        ),
                      )
                      .catch((error) =>
                        setMessage(
                          error instanceof Error
                            ? error.message
                            : "Disconnect failed.",
                        ),
                      )
                  }
                  type="button"
                >
                  Disconnect STA
                </button>
                <button
                  className="button secondary"
                  disabled={busy}
                  onClick={() => {
                    if (
                      !window.confirm(
                        "Clear saved upstream Wi-Fi credentials from the device?",
                      )
                    ) {
                      return;
                    }
                    void request("clear_sta")
                      .then(() => refreshStatus())
                      .then(() =>
                        setMessage(
                          "Saved Wi-Fi cleared. Rescan and connect again.",
                        ),
                      )
                      .catch((error) =>
                        setMessage(
                          error instanceof Error
                            ? error.message
                            : "Clear failed.",
                        ),
                      );
                  }}
                  type="button"
                >
                  Clear saved Wi-Fi
                </button>
                <button
                  className="button white-btn"
                  disabled={busy}
                  onClick={() =>
                    void request("celebrate")
                      .then(() =>
                        setMessage(
                          "LED celebrate sent — watch the blue LED near the ESP32 antenna/chip.",
                        ),
                      )
                      .catch((error) =>
                        setMessage(
                          error instanceof Error
                            ? error.message
                            : "LED command failed.",
                        ),
                      )
                  }
                  type="button"
                >
                  Blink LED
                </button>
                <button
                  className="button secondary"
                  disabled={busy}
                  onClick={() =>
                    void request("reboot")
                      .then(() =>
                        setMessage("Rebooting… reconnect USB after boot."),
                      )
                      .catch((error) =>
                        setMessage(
                          error instanceof Error
                            ? error.message
                            : "Reboot failed.",
                        ),
                      )
                  }
                  type="button"
                >
                  Restart
                </button>
              </div>
            </section>
          )}

          {tab === "scan" && (
            <section className="usb-dash-panel">
              <div className="usb-dash-row">
                <button
                  className="button secondary"
                  disabled={busy || scanning}
                  onClick={() => void refreshScan(true).catch((error) => {
                    setMessage(
                      error instanceof Error ? error.message : "Scan failed.",
                    );
                  })}
                  type="button"
                >
                  {scanning ? (
                    <LoaderCircle className="spin" size={15} />
                  ) : (
                    <RefreshCw size={15} />
                  )}
                  Refresh
                </button>
                <span className="muted">
                  {scanning ? "Scanning…" : `${networks.length} networks`}
                </span>
              </div>
              <div className="usb-dash-list">
                {networks.map((net) => (
                  <button
                    className="usb-dash-item"
                    key={`${net.ssid}-${net.channel}-${net.rssi}`}
                    onClick={() => {
                      setModalSsid(net.ssid);
                      setModalPass("");
                      setModalStatus("");
                    }}
                    type="button"
                  >
                    <span>
                      <Wifi size={15} /> {net.ssid || "(hidden)"}
                    </span>
                    <em>
                      {net.rssi} dBm · {encryptionLabel(net.encryption)}
                    </em>
                  </button>
                ))}
              </div>
            </section>
          )}

          {tab === "clients" && (
            <section className="usb-dash-panel">
              <div className="usb-dash-row">
                <button
                  className="button secondary"
                  disabled={busy}
                  onClick={() => void refreshClients()}
                  type="button"
                >
                  <RefreshCw size={15} /> Refresh
                </button>
              </div>
              <div className="usb-dash-list">
                {clients.length === 0 && (
                  <p className="muted">No SoftAP clients connected.</p>
                )}
                {clients.map((client, index) => (
                  <div className="usb-dash-item static" key={client.mac ?? index}>
                    <span>{client.mac || "unknown"}</span>
                    <em>{client.ip || "—"}</em>
                  </div>
                ))}
              </div>
            </section>
          )}

          {tab === "settings" && (
            <section className="usb-dash-panel">
              <form className="usb-dash-form" onSubmit={(e) => void saveSettings(e)}>
                <label>
                  AP Name
                  <input
                    maxLength={32}
                    onChange={(e) =>
                      setSettings((s) => ({ ...s, apSsid: e.target.value }))
                    }
                    required
                    value={settings.apSsid}
                  />
                </label>
                <label>
                  AP Password
                  <input
                    maxLength={63}
                    minLength={8}
                    onChange={(e) =>
                      setSettings((s) => ({ ...s, apPass: e.target.value }))
                    }
                    required
                    type="password"
                    value={settings.apPass}
                  />
                </label>
                <label>
                  Device Name
                  <input
                    maxLength={32}
                    onChange={(e) =>
                      setSettings((s) => ({ ...s, deviceName: e.target.value }))
                    }
                    required
                    value={settings.deviceName}
                  />
                </label>
                <label>
                  Hostname
                  <input
                    maxLength={32}
                    onChange={(e) =>
                      setSettings((s) => ({ ...s, hostname: e.target.value }))
                    }
                    required
                    value={settings.hostname}
                  />
                </label>
                <button className="button primary" disabled={busy} type="submit">
                  Save
                </button>
              </form>
            </section>
          )}

          {tab === "system" && (
            <section className="usb-dash-panel">
              <div className="usb-dash-grid">
                <div className="usb-dash-card">
                  <small>Firmware</small>
                  <strong>{sys?.firmware ?? fw}</strong>
                </div>
                <div className="usb-dash-card">
                  <small>SDK</small>
                  <strong>{sys?.sdk ?? "—"}</strong>
                </div>
                <div className="usb-dash-card">
                  <small>CPU</small>
                  <strong>
                    {sys?.cpuMhz != null ? `${sys.cpuMhz} MHz` : "—"}
                  </strong>
                </div>
                <div className="usb-dash-card">
                  <small>Flash</small>
                  <strong>
                    {sys?.flashSize != null
                      ? `${Math.round(sys.flashSize / 1048576)} MB`
                      : "—"}
                  </strong>
                </div>
                <div className="usb-dash-card">
                  <small>Reset</small>
                  <strong>{sys?.resetReason ?? "—"}</strong>
                </div>
                <div className="usb-dash-card">
                  <small>Crash</small>
                  <strong>
                    {sys?.crashPending ? sys.crashText || "pending" : "none"}
                  </strong>
                </div>
                <div className="usb-dash-card wide">
                  <small>Health</small>
                  <strong>
                    {sys?.health?.detail || sys?.health?.state || "—"}
                  </strong>
                </div>
              </div>
              <div className="usb-dash-row">
                <button
                  className="button secondary"
                  disabled={busy}
                  onClick={() =>
                    void request("health")
                      .then(() => refreshStatus())
                      .then(() => setMessage("Health check requested."))
                  }
                  type="button"
                >
                  Run health check
                </button>
                <button
                  className="button secondary"
                  disabled={busy}
                  onClick={() => {
                    if (
                      !window.confirm(
                        "Factory reset clears Wi-Fi and AP settings. Continue?",
                      )
                    ) {
                      return;
                    }
                    void request("factory_reset", { confirm: true })
                      .then(() =>
                        setMessage("Factory reset done. Reconnect after reboot."),
                      )
                      .catch((error) =>
                        setMessage(
                          error instanceof Error
                            ? error.message
                            : "Factory reset failed.",
                        ),
                      );
                  }}
                  type="button"
                >
                  Factory Reset
                </button>
              </div>
              <p className="muted" style={{ marginTop: 16 }}>
                OTA uploads still use SoftAP dashboard or the browser installer.
              </p>
            </section>
          )}

          {tab === "logs" && (
            <section className="usb-dash-panel">
              <div className="usb-dash-row">
                <button
                  className="button secondary"
                  disabled={busy}
                  onClick={() => void refreshLogs()}
                  type="button"
                >
                  <RefreshCw size={15} /> Refresh
                </button>
              </div>
              <pre className="usb-dash-logs">{logs || "No logs yet."}</pre>
            </section>
          )}
        </>
      )}

      {modalSsid && (
        <div className="usb-dash-modal" role="dialog" aria-modal="true">
          <div className="usb-dash-modal-card">
            <small>Connect</small>
            <strong>{modalSsid}</strong>
            <input
              autoComplete="new-password"
              maxLength={63}
              minLength={8}
              onChange={(e) => setModalPass(e.target.value)}
              placeholder="Password"
              type="password"
              value={modalPass}
            />
            <div className="usb-dash-row">
              <button
                className="button primary"
                disabled={busy || modalPass.length < 8}
                onClick={() => void connectWifi()}
                type="button"
              >
                Connect
              </button>
              <button
                className="button secondary"
                onClick={() => {
                  setModalSsid(null);
                  setModalPass("");
                  setModalStatus("");
                }}
                type="button"
              >
                Cancel
              </button>
            </div>
            {modalStatus && <p className="muted">{modalStatus}</p>}
          </div>
        </div>
      )}
    </div>
  );
}

export default UsbSetup;
