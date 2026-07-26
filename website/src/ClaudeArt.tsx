/** Soft live SVG atmosphere — Claude-like paper + signal geometry. */
export function ClaudeArt({ className = "" }: { className?: string }) {
  return (
    <div className={`claude-art ${className}`} aria-hidden="true">
      <svg className="claude-art-svg" viewBox="0 0 1200 800" fill="none">
        <defs>
          <linearGradient id="neWash" x1="0" y1="0" x2="1" y2="1">
            <stop offset="0%" stopColor="var(--art-a)" stopOpacity="0.55" />
            <stop offset="55%" stopColor="var(--art-b)" stopOpacity="0.28" />
            <stop offset="100%" stopColor="var(--art-c)" stopOpacity="0.18" />
          </linearGradient>
          <radialGradient id="neGlow" cx="50%" cy="40%" r="50%">
            <stop offset="0%" stopColor="var(--art-accent)" stopOpacity="0.35" />
            <stop offset="100%" stopColor="var(--art-accent)" stopOpacity="0" />
          </radialGradient>
        </defs>

        <rect width="1200" height="800" fill="url(#neWash)" />
        <circle className="art-pulse" cx="920" cy="160" r="180" fill="url(#neGlow)" />
        <circle className="art-pulse delay" cx="180" cy="620" r="140" fill="url(#neGlow)" />

        <g className="art-orbit" stroke="var(--art-line)" strokeWidth="1.2">
          <ellipse cx="600" cy="380" rx="340" ry="160" opacity="0.35" />
          <ellipse cx="600" cy="380" rx="260" ry="120" opacity="0.28" />
          <ellipse cx="600" cy="380" rx="180" ry="80" opacity="0.22" />
        </g>

        <g className="art-nodes" fill="var(--art-accent)">
          <circle className="art-node" cx="340" cy="300" r="5" />
          <circle className="art-node delay" cx="600" cy="250" r="4" />
          <circle className="art-node" cx="820" cy="340" r="5" />
          <circle className="art-node delay" cx="520" cy="480" r="4" />
          <circle className="art-node" cx="740" cy="470" r="4.5" />
        </g>

        <path
          className="art-trace"
          d="M280 520 C 380 420, 460 560, 560 470 S 760 360, 880 430"
          stroke="var(--art-line)"
          strokeWidth="1.4"
          strokeDasharray="6 10"
        />
        <path
          className="art-trace reverse"
          d="M220 240 C 360 280, 480 180, 640 220 S 900 300, 980 240"
          stroke="var(--art-accent)"
          strokeWidth="1.2"
          strokeDasharray="4 12"
          opacity="0.7"
        />

        <g opacity="0.5" stroke="var(--art-line)" strokeWidth="1">
          <path d="M80 120h40M100 100v40" />
          <path d="M1080 640h40M1100 620v40" />
          <path d="M140 680h28M154 666v28" />
        </g>
      </svg>
    </div>
  );
}

export function SignalMark({ size = 28 }: { size?: number }) {
  return (
    <svg
      width={size}
      height={size}
      viewBox="0 0 48 48"
      fill="none"
      aria-hidden="true"
    >
      <rect
        x="6"
        y="6"
        width="36"
        height="36"
        rx="12"
        stroke="currentColor"
        strokeWidth="2"
        opacity="0.35"
      />
      <path
        d="M16 30 L24 14 L32 30"
        stroke="currentColor"
        strokeWidth="2.4"
        strokeLinecap="round"
        strokeLinejoin="round"
      />
      <circle cx="18" cy="28" r="2.2" fill="currentColor" />
      <circle cx="30" cy="28" r="2.2" fill="currentColor" />
      <path
        className="art-trace"
        d="M14 34c4 3 16 3 20 0"
        stroke="currentColor"
        strokeWidth="1.6"
        strokeLinecap="round"
        opacity="0.55"
      />
    </svg>
  );
}
