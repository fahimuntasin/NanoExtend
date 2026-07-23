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
      <path
        className="logo-frame"
        d="M22 76V24L68 72V20"
        stroke="currentColor"
        strokeLinecap="round"
        strokeLinejoin="round"
        strokeWidth="9"
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
    </svg>
  );
}

export default Logo;
