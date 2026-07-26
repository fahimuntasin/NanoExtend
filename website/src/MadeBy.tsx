export function MadeBy({ compact = false }: { compact?: boolean }) {
  return (
    <p className={`made-by ${compact ? "compact" : ""}`}>
      Made by{" "}
      <a href="https://fahimuntasin.com" rel="noreferrer" target="_blank">
        fahimuntasin.com
      </a>
      {!compact && (
        <span> — a tech enthusiast from an early age.</span>
      )}
    </p>
  );
}
