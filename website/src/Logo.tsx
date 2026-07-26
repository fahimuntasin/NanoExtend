type LogoProps = {
  size?: number;
  animated?: boolean;
  className?: string;
};

function Logo({ size = 36, animated = false, className = "" }: LogoProps) {
  return (
    <svg
      aria-label="NanoExtend"
      className={`nano-logo ${animated ? "nano-logo-animated" : ""} ${className}`}
      fill="none"
      height={size}
      role="img"
      viewBox="0 0 96 96"
      width={size}
    >
      <defs>
        <radialGradient id="nanoGlow" cx="70%" cy="28%" r="45%">
          <stop offset="0%" stopColor="currentColor" stopOpacity="0.28" />
          <stop offset="100%" stopColor="currentColor" stopOpacity="0" />
        </radialGradient>
      </defs>

      <circle
        className="logo-aura"
        cx="70"
        cy="28"
        fill="url(#nanoGlow)"
        r="34"
      />

      {/* Circuit N mark */}
      <path
        className="logo-frame"
        d="M22 76V24L68 72V20"
        stroke="currentColor"
        strokeLinecap="round"
        strokeLinejoin="round"
        strokeWidth="9"
      />
      <path
        className="logo-trace"
        d="M18 68 L30 68 L36 74"
        stroke="currentColor"
        strokeLinecap="round"
        strokeLinejoin="round"
        strokeWidth="4"
      />

      <circle
        className="logo-node logo-node-a"
        cx="22"
        cy="20"
        r="8"
        stroke="currentColor"
        strokeWidth="7"
      />
      <circle
        className="logo-node logo-node-b"
        cx="68"
        cy="76"
        r="8"
        stroke="currentColor"
        strokeWidth="7"
      />
      <circle
        className="logo-core"
        cx="22"
        cy="20"
        fill="currentColor"
        r="2.5"
      />
      <circle
        className="logo-core logo-core-b"
        cx="68"
        cy="76"
        fill="currentColor"
        r="2.5"
      />

      {/* Wi-Fi arcs */}
      <path
        className="logo-wave wave-one"
        d="M56 20c9 0 16 7 16 16"
        stroke="currentColor"
        strokeLinecap="round"
        strokeWidth="6"
      />
      <path
        className="logo-wave wave-two"
        d="M56 8c16 0 28 12 28 28"
        stroke="currentColor"
        strokeLinecap="round"
        strokeWidth="6"
      />
      <path
        className="logo-wave wave-three"
        d="M56 -2c22 0 38 16 38 38"
        stroke="currentColor"
        strokeLinecap="round"
        strokeWidth="5"
      />
    </svg>
  );
}

export default Logo;
