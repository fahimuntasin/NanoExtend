#include "web_server.h"

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <esp_random.h>
#include <new>

#include "captive_portal.h"
#include "config.h"
#include "logger.h"
#include "ota.h"
#include "storage.h"
#include "system.h"
#include "version.h"
#include "web_assets.h"
#include "wifi_manager.h"

namespace {
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

char g_session[33] = {0};
char g_csrf[CFG_CSRF_LEN + 1] = {0};
uint32_t g_sessionMs = 0;
uint32_t g_rateWindowMs = 0;
uint8_t g_rateCount = 0;
uint32_t g_lastWsMs = 0;
bool g_jobBusy = false;

void randomToken(char* out, size_t len) {
  static const char* alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  for (size_t i = 0; i + 1 < len; i++) {
    out[i] = alphabet[esp_random() % 62];
  }
  out[len - 1] = '\0';
}

void ensureSession() {
  if (g_session[0] == '\0' || (millis() - g_sessionMs) > CFG_SESSION_TTL_MS) {
    randomToken(g_session, sizeof(g_session));
    randomToken(g_csrf, sizeof(g_csrf));
    g_sessionMs = millis();
    LOG_I("Web", "New admin session issued");
  }
}

bool rateLimitOk() {
  uint32_t now = millis();
  if (now - g_rateWindowMs > CFG_RATE_LIMIT_WINDOW_MS) {
    g_rateWindowMs = now;
    g_rateCount = 0;
  }
  if (g_rateCount >= CFG_RATE_LIMIT_MAX)
    return false;
  g_rateCount++;
  return true;
}

bool fromApSubnet(AsyncWebServerRequest* req) {
  if (!req->client())
    return false;
  IPAddress ip = req->client()->remoteIP();
  // SoftAP clients are typically 192.168.4.x
  return ip[0] == 192 && ip[1] == 168 && ip[2] == 4;
}

bool requireApClient(AsyncWebServerRequest* req) {
  if (fromApSubnet(req))
    return true;
  req->send(403, "application/json", "{\"ok\":false,\"error\":\"AP clients only\"}");
  return false;
}

bool requireCsrf(AsyncWebServerRequest* req) {
  ensureSession();
  if (!req->hasHeader("X-CSRF-Token") || !req->hasHeader("X-Session")) {
    req->send(403, "application/json", "{\"ok\":false,\"error\":\"session and CSRF required\"}");
    return false;
  }
  String token = req->getHeader("X-CSRF-Token")->value();
  String session = req->getHeader("X-Session")->value();
  if (token != String(g_csrf) || session != String(g_session)) {
    req->send(403, "application/json", "{\"ok\":false,\"error\":\"session or CSRF invalid\"}");
    return false;
  }
  g_sessionMs = millis();
  return true;
}

bool sanitizeSsid(const char* ssid) {
  if (!ssid)
    return false;
  size_t n = strlen(ssid);
  if (n < 1 || n > CFG_SSID_MAX)
    return false;
  for (size_t i = 0; i < n; i++) {
    if (static_cast<uint8_t>(ssid[i]) < 0x20)
      return false;
  }
  return true;
}

bool sanitizePass(const char* pass) {
  if (!pass)
    return false;
  size_t n = strlen(pass);
  if (n < CFG_PASS_MIN || n > CFG_PASS_MAX)
    return false;
  for (size_t i = 0; i < n; i++) {
    if (static_cast<uint8_t>(pass[i]) < 0x20)
      return false;
  }
  return true;
}

bool sanitizeName(const char* name, size_t maxLen) {
  if (!name)
    return false;
  size_t n = strlen(name);
  if (n < 1 || n > maxLen)
    return false;
  for (size_t i = 0; i < n; i++) {
    char c = name[i];
    if (!(isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == ' '))
      return false;
  }
  return true;
}

String jsonError(const char* err) {
  JsonDocument doc;
  doc["ok"] = false;
  doc["error"] = err;
  ensureSession();
  doc["csrf"] = g_csrf;
  doc["session"] = g_session;
  String out;
  serializeJson(doc, out);
  return out;
}

void sendGzip(AsyncWebServerRequest* req, const char* mime, const uint8_t* data, size_t len) {
  AsyncWebServerResponse* res = req->beginResponse(200, mime, data, len);
  res->addHeader("Content-Encoding", "gzip");
  res->addHeader("Cache-Control", "no-store");
  req->send(res);
}

String buildStatusJson() {
  JsonDocument doc;
  doc["ok"] = true;
  ensureSession();
  doc["csrf"] = g_csrf;
  doc["session"] = g_session;
  JsonObject wifi = doc["wifi"].to<JsonObject>();
  WifiManager::fillStatusJson(wifi);
  JsonObject sys = doc["system"].to<JsonObject>();
  SystemInfo::fillStatusJson(sys);
  JsonObject ota = doc["ota"].to<JsonObject>();
  OtaManager::fillJson(ota);
  String out;
  serializeJson(doc, out);
  return out;
}

void handleStatus(AsyncWebServerRequest* req) {
  if (!requireApClient(req))
    return;
  if (!rateLimitOk()) {
    req->send(429, "application/json", jsonError("rate limited"));
    return;
  }
  req->send(200, "application/json", buildStatusJson());
}

void handleScan(AsyncWebServerRequest* req) {
  if (!requireApClient(req))
    return;
  if (!rateLimitOk()) {
    req->send(429, "application/json", jsonError("rate limited"));
    return;
  }
  bool refresh = req->hasParam("refresh");
  if (refresh) {
    if (g_jobBusy) {
      req->send(409, "application/json", jsonError("busy"));
      return;
    }
    WifiManager::startScan(true);
  } else if (!WifiManager::scanReady()) {
    WifiManager::startScan(false);
  }

  JsonDocument doc;
  doc["ok"] = true;
  ensureSession();
  doc["csrf"] = g_csrf;
  doc["session"] = g_session;
  doc["cached"] = WifiManager::scanReady();
  JsonObject tmp;
  // age from wifi status
  {
    JsonDocument st;
    JsonObject w = st.to<JsonObject>();
    WifiManager::fillStatusJson(w);
    doc["ageMs"] = w["scanAgeMs"];
    doc["inProgress"] = w["scanInProgress"];
  }
  JsonArray arr = doc["networks"].to<JsonArray>();
  size_t n = WifiManager::scanCount();
  for (size_t i = 0; i < n; i++) {
    ScanNetwork sn;
    if (!WifiManager::scanItem(i, sn))
      continue;
    JsonObject o = arr.add<JsonObject>();
    o["ssid"] = sn.ssid;
    o["rssi"] = sn.rssi;
    o["channel"] = sn.channel;
    o["encryption"] = sn.enc;
  }
  String out;
  serializeJson(doc, out);
  req->send(200, "application/json", out);
}

void handleClients(AsyncWebServerRequest* req) {
  if (!requireApClient(req))
    return;
  JsonDocument doc;
  doc["ok"] = true;
  ensureSession();
  doc["csrf"] = g_csrf;
  doc["session"] = g_session;
  JsonArray arr = doc["clients"].to<JsonArray>();
  WifiManager::fillClientsJson(arr);
  String out;
  serializeJson(doc, out);
  req->send(200, "application/json", out);
}

void handleGetSettings(AsyncWebServerRequest* req) {
  if (!requireApClient(req))
    return;
  DeviceSettings s = Storage::load();
  JsonDocument doc;
  doc["ok"] = true;
  ensureSession();
  doc["csrf"] = g_csrf;
  doc["session"] = g_session;
  doc["apSsid"] = s.apSsid;
  doc["apPass"] = s.apPass; // local AP password only; never STA password
  doc["deviceName"] = s.deviceName;
  doc["hostname"] = s.hostname;
  doc["hasStaCreds"] = s.hasStaCreds;
  doc["staSsid"] = s.hasStaCreds ? s.staSsid : "";
  String out;
  serializeJson(doc, out);
  req->send(200, "application/json", out);
}

void handleBodyJson(AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index,
                    size_t total, std::function<void(JsonDocument&)> fn);

void handleBackup(AsyncWebServerRequest* req) {
  if (!requireApClient(req) || !requireCsrf(req))
    return;
  DeviceSettings s = Storage::load();
  JsonDocument doc;
  doc["format"] = "nanoextend-settings";
  doc["version"] = 1;
  doc["createdBy"] = NANOEXTEND_FW_VERSION;
  JsonObject settings = doc["settings"].to<JsonObject>();
  settings["apSsid"] = s.apSsid;
  settings["apPass"] = s.apPass;
  settings["deviceName"] = s.deviceName;
  settings["hostname"] = s.hostname;
  settings["staSsid"] = s.hasStaCreds ? s.staSsid : "";
  settings["staPass"] = s.hasStaCreds ? s.staPass : "";
  settings["autoReconnect"] = s.autoReconnect;
  settings["hasStaCreds"] = s.hasStaCreds;
  String out;
  serializeJsonPretty(doc, out);
  AsyncWebServerResponse* res = req->beginResponse(200, "application/json", out);
  res->addHeader("Content-Disposition", "attachment; filename=nanoextend-settings.json");
  res->addHeader("Cache-Control", "no-store");
  req->send(res);
}

void handleRestore(AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index,
                   size_t total) {
  if (!requireApClient(req) || !requireCsrf(req))
    return;
  handleBodyJson(req, data, len, index, total, [req](JsonDocument& doc) {
    if (String(doc["format"] | "") != "nanoextend-settings" || (doc["version"] | 0) != 1 ||
        !doc["settings"].is<JsonObject>()) {
      req->send(400, "application/json", jsonError("unsupported backup format"));
      return;
    }

    JsonObject in = doc["settings"].as<JsonObject>();
    const char* apSsid = in["apSsid"] | "";
    const char* apPass = in["apPass"] | "";
    const char* deviceName = in["deviceName"] | "";
    const char* hostname = in["hostname"] | "";
    bool hasSta = in["hasStaCreds"] | false;
    const char* staSsid = in["staSsid"] | "";
    const char* staPass = in["staPass"] | "";
    if (!sanitizeSsid(apSsid) || !sanitizePass(apPass) ||
        !sanitizeName(deviceName, CFG_DEVICE_NAME_MAX) ||
        !sanitizeName(hostname, CFG_HOSTNAME_MAX) ||
        (hasSta && (!sanitizeSsid(staSsid) || !sanitizePass(staPass)))) {
      req->send(400, "application/json", jsonError("backup contains invalid settings"));
      return;
    }

    DeviceSettings s = Storage::load();
    strlcpy(s.apSsid, apSsid, sizeof(s.apSsid));
    strlcpy(s.apPass, apPass, sizeof(s.apPass));
    strlcpy(s.deviceName, deviceName, sizeof(s.deviceName));
    strlcpy(s.hostname, hostname, sizeof(s.hostname));
    s.hasStaCreds = hasSta;
    s.autoReconnect = in["autoReconnect"] | true;
    if (hasSta) {
      strlcpy(s.staSsid, staSsid, sizeof(s.staSsid));
      strlcpy(s.staPass, staPass, sizeof(s.staPass));
    } else {
      s.staSsid[0] = '\0';
      s.staPass[0] = '\0';
    }
    s.checksum = 0;
    if (!Storage::save(s)) {
      req->send(500, "application/json", jsonError("restore failed"));
      return;
    }
    req->send(200, "application/json", "{\"ok\":true,\"restartRequired\":true}");
  });
}

void handleLogs(AsyncWebServerRequest* req) {
  if (!requireApClient(req))
    return;
  String text = Logger::exportText();
  if (req->hasParam("export")) {
    AsyncWebServerResponse* res = req->beginResponse(200, "text/plain", text);
    res->addHeader("Content-Disposition", "attachment; filename=nanoextend.log");
    req->send(res);
    return;
  }
  JsonDocument doc;
  doc["ok"] = true;
  ensureSession();
  doc["csrf"] = g_csrf;
  doc["session"] = g_session;
  doc["count"] = Logger::count();
  doc["text"] = text;
  String out;
  serializeJson(doc, out);
  req->send(200, "application/json", out);
}

void handleBodyJson(AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index,
                    size_t total, std::function<void(JsonDocument&)> fn) {
  if (index == 0) {
    if (total > 2048) {
      req->send(413, "application/json", jsonError("body too large"));
      return;
    }
    auto* body = new (std::nothrow) String();
    if (!body || !body->reserve(total)) {
      delete body;
      req->send(503, "application/json", jsonError("insufficient memory"));
      return;
    }
    req->_tempObject = body;
  }
  auto* body = static_cast<String*>(req->_tempObject);
  if (!body)
    return;
  body->concat(reinterpret_cast<const char*>(data), len);
  if (index + len != total)
    return;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, *body);
  delete body;
  req->_tempObject = nullptr;
  if (err) {
    req->send(400, "application/json", jsonError("invalid json"));
    return;
  }
  fn(doc);
}

void handleConnect(AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index,
                   size_t total) {
  if (!requireApClient(req) || !requireCsrf(req))
    return;
  if (!rateLimitOk()) {
    req->send(429, "application/json", jsonError("rate limited"));
    return;
  }
  handleBodyJson(req, data, len, index, total, [req](JsonDocument& doc) {
    if (g_jobBusy || OtaManager::isBusy()) {
      req->send(409, "application/json", jsonError("busy"));
      return;
    }
    const char* ssid = doc["ssid"] | "";
    const char* pass = doc["password"] | "";
    if (!sanitizeSsid(ssid) || !sanitizePass(pass)) {
      req->send(400, "application/json", jsonError("invalid ssid/password"));
      return;
    }
    g_jobBusy = true;
    bool ok = WifiManager::connect(ssid, pass, true);
    g_jobBusy = false;
    if (!ok) {
      req->send(400, "application/json", jsonError(SystemInfo::lastError()));
      return;
    }
    JsonDocument out;
    out["ok"] = true;
    ensureSession();
    out["csrf"] = g_csrf;
    out["session"] = g_session;
    String s;
    serializeJson(out, s);
    req->send(200, "application/json", s);
  });
}

void handleDisconnect(AsyncWebServerRequest* req) {
  if (!requireApClient(req) || !requireCsrf(req))
    return;
  WifiManager::disconnectSta();
  req->send(200, "application/json", buildStatusJson());
}

void handlePostSettings(AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index,
                        size_t total) {
  if (!requireApClient(req) || !requireCsrf(req))
    return;
  handleBodyJson(req, data, len, index, total, [req](JsonDocument& doc) {
    const char* apSsid = doc["apSsid"] | "";
    const char* apPass = doc["apPass"] | "";
    const char* deviceName = doc["deviceName"] | "";
    const char* hostname = doc["hostname"] | "";
    if (!sanitizeSsid(apSsid) || !sanitizePass(apPass) ||
        !sanitizeName(deviceName, CFG_DEVICE_NAME_MAX) ||
        !sanitizeName(hostname, CFG_HOSTNAME_MAX)) {
      req->send(400, "application/json", jsonError("invalid settings"));
      return;
    }
    DeviceSettings s = Storage::load();
    strncpy(s.apSsid, apSsid, sizeof(s.apSsid) - 1);
    strncpy(s.apPass, apPass, sizeof(s.apPass) - 1);
    strncpy(s.deviceName, deviceName, sizeof(s.deviceName) - 1);
    strncpy(s.hostname, hostname, sizeof(s.hostname) - 1);
    s.checksum = 0;
    if (!Storage::save(s)) {
      req->send(500, "application/json", jsonError("save failed"));
      return;
    }
    WifiManager::applyApSettings(Storage::load());
    req->send(200, "application/json", "{\"ok\":true}");
  });
}

void handleReboot(AsyncWebServerRequest* req) {
  if (!requireApClient(req) || !requireCsrf(req))
    return;
  if (OtaManager::isBusy()) {
    req->send(409, "application/json", jsonError("ota busy"));
    return;
  }
  req->send(200, "application/json", "{\"ok\":true}");
  delay(200);
  ESP.restart();
}

void handleReset(AsyncWebServerRequest* req) {
  if (!requireApClient(req) || !requireCsrf(req))
    return;
  Storage::factoryReset();
  req->send(200, "application/json", "{\"ok\":true}");
  delay(200);
  ESP.restart();
}

void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg,
               uint8_t* data, size_t len) {
  (void)server;
  (void)arg;
  (void)data;
  (void)len;
  if (type == WS_EVT_CONNECT) {
    client->text(buildStatusJson());
  }
}

void maybeBroadcastWs() {
  if (millis() - g_lastWsMs < CFG_WS_INTERVAL_MS)
    return;
  g_lastWsMs = millis();
  if (ws.count() == 0)
    return;
  if (ws.availableForWriteAll() < 1) {
    ws.cleanupClients(2);
    return;
  }
  String payload = buildStatusJson();
  ws.textAll(payload);
}

void setupCaptiveRoutes() {
  auto redirect = [](AsyncWebServerRequest* req) {
    if (!CaptivePortal::isActive()) {
      req->send(204);
      return;
    }
    req->redirect("http://192.168.4.1/");
  };
  server.on("/generate_204", HTTP_ANY, redirect);
  server.on("/gen_204", HTTP_ANY, redirect);
  server.on("/hotspot-detect.html", HTTP_ANY, redirect);
  server.on("/library/test/success.html", HTTP_ANY, redirect);
  server.on("/ncsi.txt", HTTP_ANY, redirect);
  server.on("/connecttest.txt", HTTP_ANY, redirect);
  server.on("/canonical.html", HTTP_ANY, redirect);
  server.on("/success.txt", HTTP_ANY, redirect);
}

void setupOtaRoute() {
  server.on(
      "/api/v1/update", HTTP_POST,
      [](AsyncWebServerRequest* req) {
        if (!requireApClient(req) || !requireCsrf(req))
          return;
        if (!OtaManager::isBusy() && OtaManager::progress() == 100 &&
            String(OtaManager::status()) == "success") {
          req->send(200, "application/json", "{\"ok\":true,\"status\":\"success\"}");
          delay(300);
          ESP.restart();
          return;
        }
        if (String(OtaManager::status()) == "success") {
          req->send(200, "application/json", "{\"ok\":true}");
          return;
        }
        req->send(400, "application/json", jsonError(OtaManager::status()));
      },
      nullptr,
      [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
        if (index == 0) {
          if (!requireApClient(req) || !requireCsrf(req))
            return;
          if (g_jobBusy) {
            req->send(409, "application/json", jsonError("busy"));
            return;
          }
          size_t declared = total;
          if (req->hasHeader("X-File-Size")) {
            declared = req->getHeader("X-File-Size")->value().toInt();
          }
          OtaManager::setExpectedSha256(
              req->hasHeader("X-SHA256") ? req->getHeader("X-SHA256")->value().c_str() : nullptr);
          if (!OtaManager::beginUpdate(declared ? declared : total)) {
            req->send(400, "application/json", jsonError(OtaManager::status()));
            return;
          }
          g_jobBusy = true;
        }
        if (!OtaManager::isBusy())
          return;
        if (!OtaManager::writeChunk(data, len)) {
          g_jobBusy = false;
          req->send(500, "application/json", jsonError(OtaManager::status()));
          return;
        }
        if (index + len == total) {
          bool ok = OtaManager::endUpdate(true);
          g_jobBusy = false;
          if (!ok) {
            req->send(500, "application/json", jsonError(OtaManager::status()));
          }
        }
      });
}
} // namespace

void WebServerApp::begin() {
  ensureSession();
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    sendGzip(req, INDEX_HTML_MIME, INDEX_HTML_GZ, INDEX_HTML_GZ_LEN);
  });
  server.on("/app.css", HTTP_GET, [](AsyncWebServerRequest* req) {
    sendGzip(req, APP_CSS_MIME, APP_CSS_GZ, APP_CSS_GZ_LEN);
  });
  server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest* req) {
    sendGzip(req, APP_JS_MIME, APP_JS_GZ, APP_JS_GZ_LEN);
  });

  setupCaptiveRoutes();

  server.on("/api/v1/status", HTTP_GET, handleStatus);
  server.on("/api/v1/scan", HTTP_GET, handleScan);
  server.on("/api/v1/clients", HTTP_GET, handleClients);
  server.on("/api/v1/settings", HTTP_GET, handleGetSettings);
  server.on("/api/v1/settings/backup", HTTP_GET, handleBackup);
  server.on("/api/v1/logs", HTTP_GET, handleLogs);
  server.on("/api/v1/disconnect", HTTP_POST, handleDisconnect);
  server.on("/api/v1/reboot", HTTP_POST, handleReboot);
  server.on("/api/v1/reset", HTTP_POST, handleReset);
  server.on(
      "/api/v1/connect", HTTP_POST, [](AsyncWebServerRequest* req) {}, nullptr,
      [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
        handleConnect(req, data, len, index, total);
      });
  server.on(
      "/api/v1/settings", HTTP_POST, [](AsyncWebServerRequest* req) {}, nullptr,
      [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
        handlePostSettings(req, data, len, index, total);
      });
  server.on(
      "/api/v1/settings/restore", HTTP_POST, [](AsyncWebServerRequest* req) {}, nullptr,
      [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
        handleRestore(req, data, len, index, total);
      });

  setupOtaRoute();

  server.onNotFound([](AsyncWebServerRequest* req) {
    if (req->url().startsWith("/api/")) {
      req->send(404, "application/json", jsonError("not found"));
      return;
    }
    String host = req->host();
    if (CaptivePortal::handleCaptiveRequest(host, req->url())) {
      req->redirect("http://192.168.4.1/");
      return;
    }
    sendGzip(req, INDEX_HTML_MIME, INDEX_HTML_GZ, INDEX_HTML_GZ_LEN);
  });

  server.begin();
  LOG_I("Web", "Async server started on :80 api=/api/%s", NANOEXTEND_API_VERSION);
}

void WebServerApp::loop() {
  ws.cleanupClients();
  maybeBroadcastWs();
}
