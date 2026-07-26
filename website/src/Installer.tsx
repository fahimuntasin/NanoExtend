import { useMemo, useState } from "react";
import { Check, Cpu, LoaderCircle, Usb, Wifi } from "lucide-react";
import { ESPLoader, Transport, type IEspLoaderTerminal } from "esptool-js";
import { requestEspSerialPort, SERIAL_PORT_HINT } from "./serialPorts";

type InstallState =
  | "idle"
  | "connecting"
  | "downloading"
  | "flashing"
  | "verifying"
  | "done"
  | "error";

const stepOrder: InstallState[] = [
  "connecting",
  "downloading",
  "flashing",
  "verifying",
  "done",
];

function assetUrl(path: string): string {
  if (/^https?:\/\//i.test(path)) return path;
  const base = import.meta.env.BASE_URL;
  return `${base}${path.replace(/^\//, "")}`;
}

function Installer() {
  const [state, setState] = useState<InstallState>("idle");
  const [progress, setProgress] = useState(0);
  const [chip, setChip] = useState("Waiting for device");
  const [message, setMessage] = useState(
    "Chrome or Edge on desktop is required for Web Serial.",
  );
  const [logs, setLogs] = useState<string[]>([]);

  const supported = "serial" in navigator;
  const activeIndex = useMemo(() => stepOrder.indexOf(state), [state]);

  const addLog = (line: string) =>
    setLogs((current) => [...current.slice(-20), line]);

  const install = async () => {
    if (!supported) {
      setState("error");
      setMessage("Web Serial is not supported in this browser.");
      return;
    }

    let transport: Transport | undefined;
    try {
      setState("connecting");
      setMessage(SERIAL_PORT_HINT);
      setProgress(4);
      const port = await requestEspSerialPort();
      transport = new Transport(port, false);

      const terminal: IEspLoaderTerminal = {
        clean: () => setLogs([]),
        write: addLog,
        writeLine: addLog,
      };
      const loader = new ESPLoader({
        transport,
        baudrate: 460800,
        terminal,
      });
      const detectedChip = await loader.main();
      if (!detectedChip.toLowerCase().includes("esp32")) {
        throw new Error(`Unsupported chip: ${detectedChip}`);
      }
      setChip(detectedChip);
      setProgress(14);

      setState("downloading");
      setMessage("Downloading the verified NanoExtend factory image…");
      const manifestResponse = await fetch(assetUrl("firmware/manifest.json"), {
        cache: "no-store",
      });
      if (!manifestResponse.ok) {
        throw new Error("Firmware manifest is unavailable.");
      }
      const manifest = (await manifestResponse.json()) as {
        version: string;
        file?: string;
        sha256?: string;
        size?: number;
        factory?: { file: string; sha256: string; size: number };
      };
      const factory = manifest.factory ?? {
        file: manifest.file ?? "",
        sha256: manifest.sha256 ?? "",
        size: manifest.size ?? 0,
      };
      if (!factory.file || !factory.sha256) {
        throw new Error("The release manifest has no factory image.");
      }
      const firmwareResponse = await fetch(assetUrl(factory.file), {
        cache: "no-store",
      });
      if (!firmwareResponse.ok) {
        throw new Error("Firmware artifact is unavailable.");
      }
      const firmware = new Uint8Array(await firmwareResponse.arrayBuffer());
      if (factory.size && firmware.byteLength !== factory.size) {
        throw new Error("Firmware size does not match the release manifest.");
      }
      const digest = await crypto.subtle.digest("SHA-256", firmware);
      const digestHex = [...new Uint8Array(digest)]
        .map((byte) => byte.toString(16).padStart(2, "0"))
        .join("");
      if (
        factory.sha256 &&
        digestHex.toLowerCase() !== factory.sha256.toLowerCase()
      ) {
        throw new Error("Firmware checksum verification failed.");
      }
      setProgress(25);

      setState("flashing");
      setMessage(`Flashing NanoExtend ${manifest.version}…`);
      await loader.writeFlash({
        fileArray: [{ data: firmware, address: 0x0 }],
        flashMode: "dio",
        flashFreq: "40m",
        flashSize: "4MB",
        eraseAll: false,
        compress: true,
        reportProgress: (_fileIndex, written, total) => {
          setProgress(25 + Math.round((written / total) * 65));
        },
      });

      setState("verifying");
      setMessage("Verifying flash and restarting the ESP32…");
      setProgress(94);
      await loader.after("hard_reset");
      await transport.disconnect();
      transport = undefined;

      setState("done");
      setProgress(100);
      setMessage(
        "Installation complete. Next: open USB Setup below, or join NanoExtend Wi-Fi (changeme123) and visit http://192.168.4.1",
      );
    } catch (error) {
      setState("error");
      setMessage(
        error instanceof Error ? error.message : "Installation failed.",
      );
      addLog(
        error instanceof Error ? (error.stack ?? error.message) : String(error),
      );
      if (transport) {
        await transport.disconnect().catch(() => undefined);
      }
    }
  };

  return (
    <div className="installer-shell">
      <div className="installer-head">
        <div>
          <span className="eyebrow">Browser installer</span>
          <h3>Flash in under five minutes.</h3>
        </div>
        <div className={`device-pill ${state === "done" ? "is-ready" : ""}`}>
          <Cpu size={15} aria-hidden="true" />
          {chip}
        </div>
      </div>

      <div className="install-steps" aria-label="Installation progress">
        {["Connect", "Download", "Flash", "Verify", "Done"].map(
          (label, index) => {
            const complete = activeIndex > index || state === "done";
            const active = activeIndex === index;
            return (
              <div
                className={`install-step ${complete ? "complete" : ""} ${active ? "active" : ""}`}
                key={label}
              >
                <span>
                  {complete ? (
                    <Check size={14} aria-hidden="true" />
                  ) : active ? (
                    <LoaderCircle
                      className="spin"
                      size={14}
                      aria-hidden="true"
                    />
                  ) : (
                    index + 1
                  )}
                </span>
                {label}
              </div>
            );
          },
        )}
      </div>

      <div
        className="progress-track"
        role="progressbar"
        aria-valuenow={progress}
        aria-valuemin={0}
        aria-valuemax={100}
      >
        <span style={{ width: `${progress}%` }} />
      </div>

      <p
        className={state === "error" ? "installer-error" : "installer-message"}
      >
        {message}
      </p>

      <button
        className="button primary installer-button"
        disabled={!supported || !["idle", "done", "error"].includes(state)}
        onClick={install}
        type="button"
      >
        {state === "done" ? (
          <Wifi size={17} aria-hidden="true" />
        ) : (
          <Usb size={17} aria-hidden="true" />
        )}
        {state === "done" ? "Install again" : "Connect and install"}
      </button>

      {logs.length > 0 && (
        <details className="install-log">
          <summary>Installer log</summary>
          <pre>{logs.join("\n")}</pre>
        </details>
      )}
    </div>
  );
}

export default Installer;
