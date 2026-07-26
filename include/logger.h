#pragma once

#include "config.h"
#include <Arduino.h>

enum class LogLevel : uint8_t { Debug = 0, Info = 1, Warn = 2, Error = 3 };

struct LogEntry {
  uint32_t ms;
  LogLevel level;
  char tag[12];
  char msg[CFG_LOG_LINE_MAX];
};

class Logger {
public:
  static void begin(unsigned long baud = 115200);
  static void log(LogLevel level, const char* tag, const char* fmt, ...);
  static size_t count();
  static bool get(size_t indexFromNewest, LogEntry& out);
  static String exportText();
  static void clear();

private:
  static void push(LogLevel level, const char* tag, const char* msg);
  static const char* levelName(LogLevel level);
};

#define LOG_I(tag, fmt, ...) Logger::log(LogLevel::Info, tag, fmt, ##__VA_ARGS__)
#define LOG_W(tag, fmt, ...) Logger::log(LogLevel::Warn, tag, fmt, ##__VA_ARGS__)
#define LOG_E(tag, fmt, ...) Logger::log(LogLevel::Error, tag, fmt, ##__VA_ARGS__)
#define LOG_D(tag, fmt, ...) Logger::log(LogLevel::Debug, tag, fmt, ##__VA_ARGS__)
