#include "logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace {
LogEntry g_log[CFG_LOG_MAX];
size_t g_head = 0;
size_t g_count = 0;
portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;
} // namespace

void Logger::begin(unsigned long baud) {
  Serial.begin(baud);
  delay(50);
  LOG_I("System", "Logger ready");
}

const char* Logger::levelName(LogLevel level) {
  switch (level) {
  case LogLevel::Debug:
    return "D";
  case LogLevel::Info:
    return "I";
  case LogLevel::Warn:
    return "W";
  case LogLevel::Error:
    return "E";
  }
  return "?";
}

void Logger::push(LogLevel level, const char* tag, const char* msg) {
  portENTER_CRITICAL(&g_mux);
  LogEntry& e = g_log[g_head];
  e.ms = millis();
  e.level = level;
  strncpy(e.tag, tag ? tag : "?", sizeof(e.tag) - 1);
  e.tag[sizeof(e.tag) - 1] = '\0';
  strncpy(e.msg, msg ? msg : "", sizeof(e.msg) - 1);
  e.msg[sizeof(e.msg) - 1] = '\0';
  g_head = (g_head + 1) % CFG_LOG_MAX;
  if (g_count < CFG_LOG_MAX)
    g_count++;
  portEXIT_CRITICAL(&g_mux);
}

void Logger::log(LogLevel level, const char* tag, const char* fmt, ...) {
  char buf[CFG_LOG_LINE_MAX];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  // Never allow obvious password dumps in logs.
  if (strcasestr(buf, "password") || strcasestr(buf, "passwd") || strcasestr(buf, "psk=")) {
    strncpy(buf, "<redacted>", sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
  }

  Serial.printf("[%s][%s] %s\n", levelName(level), tag ? tag : "?", buf);
  push(level, tag, buf);
}

size_t Logger::count() {
  portENTER_CRITICAL(&g_mux);
  size_t c = g_count;
  portEXIT_CRITICAL(&g_mux);
  return c;
}

bool Logger::get(size_t indexFromNewest, LogEntry& out) {
  portENTER_CRITICAL(&g_mux);
  if (indexFromNewest >= g_count) {
    portEXIT_CRITICAL(&g_mux);
    return false;
  }
  size_t idx = (g_head + CFG_LOG_MAX - 1 - indexFromNewest) % CFG_LOG_MAX;
  out = g_log[idx];
  portEXIT_CRITICAL(&g_mux);
  return true;
}

String Logger::exportText() {
  String out;
  out.reserve(g_count * 80);
  const size_t n = count();
  for (size_t i = 0; i < n; i++) {
    LogEntry e;
    // Export oldest -> newest
    size_t fromNewest = n - 1 - i;
    if (!get(fromNewest, e))
      continue;
    char line[160];
    snprintf(line, sizeof(line), "%lu [%s][%s] %s\n", static_cast<unsigned long>(e.ms),
             levelName(e.level), e.tag, e.msg);
    out += line;
  }
  return out;
}

void Logger::clear() {
  portENTER_CRITICAL(&g_mux);
  g_head = 0;
  g_count = 0;
  portEXIT_CRITICAL(&g_mux);
}
