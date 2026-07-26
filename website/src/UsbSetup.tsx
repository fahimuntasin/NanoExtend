import { useCallback, useEffect, useRef, useState } from "react";
import {
  Check,
  LoaderCircle,
  PlugZap,
  RefreshCw,
  Unplug,
  Usb,
  Wifi,
} from "lucide-react";

type ConnState = "idle" | "connecting" | "ready" | "error";

type ScanNetwork = {
  ssid: string;
  rssi: number;
  channel: number;
  encryption: number;
};

type StatusResult = {
  apIp?: string;
  staIp?: string;
  wifi?: {
    state?: string;
    staSsid?: string;
    rssi?: number;
    sharing?: boolean;
  };
  system?: {
    freeHeap?: number;
    uptimeSec?: number;
  };
};

type NeReply = {
  v: number;
  id: number;
  ok: boolean;
  error?: string;
  result?: Record<string, unknown>;
};

function encryptionLabel(enc: number): string {
  return enc === 0 ? "Open" : "Secured";
}

function UsbSetup() {
  const [conn, setConn] = useState<ConnState>("idle");
  const [message, setMessage] = useState(
    "Connect the ESP32 over USB. Chrome or Edge on desktop required.",
  );
  const [fw, setFw] = useState("");
  const [status, setStatus] = useState<StatusResult | null>(null);
  const [networks, setNetworks] = useState<ScanNetwork[]>([]);
  const [scanning, setScanning] = useState(false);
  const [ssid, setSsid] = useState("");
  const [password, setPassword] = useState("");
  const [busy, setBusy] = useState(false);
  const [log, setLog] = useState<string[]>([]);

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

  const addLog = useCallback((line: string) => {
    setLog((current) => [...current.slice(-24), line]);
  }, []);

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

  const handleLine = useCallback(
    (line: string) => {
      if (!line.startsWith("NE{")) {
        if (line.includes("NanoExtend") || line.includes("[SerialAdmin]")) {
          addLog(line);
        }
        return;
      }
      try {
        const reply = JSON.parse(line.slice(2)) as NeReply;
        const pending = pendingRef.current.get(reply.id);
        if (!pending) return;
        window.clearTimeout(pending.timer);
        pendingRef.current.delete(reply.id);
        pending.resolve(reply);
      } catch {
        addLog(`Bad reply: ${line}`);
      }
    },
    [addLog],
  );

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
        /* port closed */
      }
    },
    [handleLine],
  );

  const request = useCallback(
    async (cmd: string, extra: Record<string, unknown> = {}, timeoutMs = 8000) => {
      const writer = writerRef.current;
      if (!writer) throw new Error("USB serial is not connected.");
      const id = nextIdRef.current++;
      const payload = JSON.stringify({ v: 1, id, cmd, ...extra });
      const line = `NE>${payload}\n`;
      addLog(`→ ${cmd}`);
      const reply = await new Promise<NeReply>((resolve, reject) => {
        const timer = window.setTimeout(() => {
          pendingRef.current.delete(id);
          reject(new Error(`Timeout waiting for ${cmd}`));
        }, timeoutMs);
        pendingRef.current.set(id, { resolve, reject, timer });
        writer
          .write(new TextEncoder().encode(line))
          .catch((error) => {
            window.clearTimeout(timer);
            pendingRef.current.delete(id);
            reject(error);
          });
      });
      if (!reply.ok) {
        throw new Error(reply.error || `${cmd} failed`);
      }
      return reply.result ?? {};
    },
    [addLog],
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

  const connectUsb = async () => {
    if (!supported) {
      setConn("error");
      setMessage("Web Serial is not supported in this browser.");
      return;
    }
    setConn("connecting");
    setMessage("Select the ESP32 USB serial port…");
    setBusy(true);
    try {
      await cleanupPort();
      const port = await navigator.serial.requestPort();
      await port.open({ baudRate: 115200 });
      portRef.current = port;
      bufferRef.current = "";
      decoderRef.current = new TextDecoder();

      if (!port.readable || !port.writable) {
        throw new Error("Serial port streams are unavailable.");
      }
      const reader = (
        port.readable as ReadableStream<Uint8Array>
      ).getReader();
      readerRef.current = reader;
      void readLoop(reader);
      writerRef.current = (
        port.writable as WritableStream<Uint8Array>
      ).getWriter();

      let hello: Record<string, unknown> | null = null;
      for (let attempt = 0; attempt < 8; attempt++) {
        try {
          hello = await request("hello", {}, 2000);
          break;
        } catch {
          await new Promise((r) => window.setTimeout(r, 400));
        }
      }
      if (!hello) throw new Error("No reply from NanoExtend. Is firmware 1.0.2+ flashed?");

      setFw(String(hello.fw ?? ""));
      setConn("ready");
      setMessage("USB setup ready. Scan Wi-Fi and connect upstream — no phone needed.");
      await refreshStatus();
      await refreshScan(true);

      pollRef.current = window.setInterval(() => {
        void refreshStatus().catch(() => undefined);
      }, 4000);
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
    setFw("");
    setMessage("Disconnected. Reconnect USB anytime to configure NanoExtend.");
    setBusy(false);
  };

  const connectWifi = async () => {
    if (!ssid.trim()) {
      setMessage("Choose or enter a Wi-Fi network.");
      return;
    }
    setBusy(true);
    setMessage(`Connecting to ${ssid}…`);
    try {
      await request("connect", { ssid: ssid.trim(), password }, 25000);
      setMessage(`Connect accepted for ${ssid}. Waiting for STA IP…`);
      for (let i = 0; i < 12; i++) {
        await new Promise((r) => window.setTimeout(r, 1000));
        const st = await refreshStatus();
        if (st.staIp) {
          setMessage(
            `Upstream connected. SoftAP dashboard remains at http://${st.apIp || "192.168.4.1"} — join the NanoExtend Wi-Fi from this PC if you want the full UI.`,
          );
          break;
        }
      }
    } catch (error) {
      setMessage(error instanceof Error ? error.message : "Connect failed.");
    } finally {
      setBusy(false);
    }
  };

  const disconnectWifi = async () => {
    setBusy(true);
    try {
      await request("disconnect");
      await refreshStatus();
      setMessage("Upstream Wi-Fi disconnected.");
    } catch (error) {
      setMessage(error instanceof Error ? error.message : "Disconnect failed.");
    } finally {
      setBusy(false);
    }
  };

  useEffect(() => {
    return () => {
      void cleanupPort();
    };
  }, [cleanupPort]);

  return (
    <div className="installer-shell usb-setup-shell">
      <div className="installer-head">
        <div>
          <span className="eyebrow">USB setup</span>
          <h3>Configure without a phone.</h3>
        </div>
        <div className={`device-pill ${conn === "ready" ? "is-ready" : ""}`}>
          <Usb size={15} aria-hidden="true" />
          {conn === "ready" ? `USB · ${fw || "ready"}` : "Waiting for USB"}
        </div>
      </div>

      <p className={conn === "error" ? "installer-error" : "installer-message"}>
        {message}
      </p>

      <div className="usb-actions">
        {conn !== "ready" ? (
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
            Connect USB and setup
          </button>
        ) : (
          <button
            className="button secondary installer-button"
            disabled={busy}
            onClick={() => void disconnectUsb()}
            type="button"
          >
            <Unplug size={17} aria-hidden="true" />
            Disconnect USB
          </button>
        )}
      </div>

      {conn === "ready" && status && (
        <div className="usb-status-grid" aria-live="polite">
          <div>
            <small>Wi-Fi state</small>
            <strong>{status.wifi?.state ?? "—"}</strong>
          </div>
          <div>
            <small>Upstream SSID</small>
            <strong>{status.wifi?.staSsid || "Not connected"}</strong>
          </div>
          <div>
            <small>STA IP</small>
            <strong>{status.staIp || "—"}</strong>
          </div>
          <div>
            <small>SoftAP IP</small>
            <strong>{status.apIp || "192.168.4.1"}</strong>
          </div>
        </div>
      )}

      {conn === "ready" && (
        <div className="usb-wifi-panel">
          <div className="usb-wifi-toolbar">
            <strong>Upstream networks</strong>
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
                <LoaderCircle className="spin" size={16} aria-hidden="true" />
              ) : (
                <RefreshCw size={16} aria-hidden="true" />
              )}
              Rescan
            </button>
          </div>

          <div className="usb-network-list" role="list">
            {networks.length === 0 && (
              <p className="usb-empty">
                {scanning ? "Scanning…" : "No networks yet. Tap Rescan."}
              </p>
            )}
            {networks.map((net) => (
              <button
                className={`usb-network ${ssid === net.ssid ? "selected" : ""}`}
                key={`${net.ssid}-${net.channel}-${net.rssi}`}
                onClick={() => setSsid(net.ssid)}
                role="listitem"
                type="button"
              >
                <span>
                  <Wifi size={15} aria-hidden="true" />
                  {net.ssid || "(hidden)"}
                </span>
                <em>
                  {net.rssi} dBm · {encryptionLabel(net.encryption)}
                </em>
              </button>
            ))}
          </div>

          <label className="usb-field">
            <span>SSID</span>
            <input
              autoComplete="off"
              onChange={(e) => setSsid(e.target.value)}
              value={ssid}
            />
          </label>
          <label className="usb-field">
            <span>Password</span>
            <input
              autoComplete="new-password"
              onChange={(e) => setPassword(e.target.value)}
              type="password"
              value={password}
            />
          </label>

          <div className="usb-actions">
            <button
              className="button primary"
              disabled={busy || !ssid.trim()}
              onClick={() => void connectWifi()}
              type="button"
            >
              <Check size={16} aria-hidden="true" />
              Connect upstream Wi-Fi
            </button>
            <button
              className="button secondary"
              disabled={busy}
              onClick={() => void disconnectWifi()}
              type="button"
            >
              Disconnect STA
            </button>
          </div>
        </div>
      )}

      {log.length > 0 && (
        <details className="install-log">
          <summary>USB serial log</summary>
          <pre>{log.join("\n")}</pre>
        </details>
      )}
    </div>
  );
}

export default UsbSetup;
