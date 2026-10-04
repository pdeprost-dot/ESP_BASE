#include "WebService.h"

#include "PlatformCompat.h"

namespace {
const char kPageStart[] PROGMEM =
    "<!doctype html><html lang='fr'><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>ESP_BASE</title><style>body{font:16px sans-serif;max-width:42rem;"
    "margin:2rem auto;padding:0 1rem;color:#222}h1{font-size:1.6rem}"
    "dl{display:grid;grid-template-columns:11rem 1fr;gap:.5rem}dt{font-weight:bold}"
    "code{overflow-wrap:anywhere}</style></head><body><h1>ESP_BASE</h1><dl>";
const char kPageEnd[] PROGMEM =
    "</dl><p><a href='/api/status'>API status</a> &middot; "
    "<a href='/api/logs'>Logs</a></p></body></html>";
const char kProvisioningForm[] PROGMEM =
    "</dl><h2>Configuration Wi-Fi</h2>"
    "<form method='post' action='/api/wifi'>"
    "<p><label>SSID<br><input name='ssid' maxlength='32' required></label></p>"
    "<p><label>Mot de passe<br><input name='password' type='password' "
    "maxlength='64' autocomplete='new-password'></label></p>"
    "<p><button type='submit'>Enregistrer et connecter</button></p></form>"
    "<p>Cette interface HTTP est réservée au réseau local de confiance.</p>"
    "<p><a href='/api/status'>API status</a> &middot; "
    "<a href='/api/logs'>Logs</a></p></body></html>";
}  // namespace

WebService::WebService() : server_(80) {}

void WebService::begin(const char* projectName, const char* firmwareVersion,
                       const DeviceIdentity& identity, ConfigStore& config,
                       WiFiService& wifi, LogService& logs) {
  projectName_ = projectName;
  firmwareVersion_ = firmwareVersion;
  identity_ = &identity;
  config_ = &config;
  wifi_ = &wifi;
  logs_ = &logs;
  heapBeforeBegin_ = PlatformCompat::freeHeap();
  registerRoutes();
  server_.begin();
  heapAfterBegin_ = PlatformCompat::freeHeap();
  minimumHeap_ = heapAfterBegin_;
  logs_->add("WEB", "Ready port=80 heap_before=%lu heap_after=%lu",
             static_cast<unsigned long>(heapBeforeBegin_),
             static_cast<unsigned long>(heapAfterBegin_));
}

void WebService::tick() {
  server_.handleClient();
  const uint32_t current = PlatformCompat::freeHeap();
  if (current < minimumHeap_) minimumHeap_ = current;
}

void WebService::registerRoutes() {
  server_.on("/", HTTP_GET, [this]() { sendHome(); });
  server_.on("/api/status", HTTP_GET, [this]() { sendStatus(); });
  server_.on("/api/logs", HTTP_GET, [this]() { sendLogs(); });
  server_.on("/api/wifi", HTTP_POST, [this]() { saveWiFi(); });
  server_.onNotFound([this]() { server_.send(404, "text/plain", "Not found\n"); });
}

void WebService::sendHtmlValue(const char* label, const char* value) {
  char escaped[129];
  htmlEscape(value, escaped, sizeof(escaped));
  char row[192];
  snprintf(row, sizeof(row), "<dt>%s</dt><dd><code>%s</code></dd>", label, escaped);
  server_.sendContent(row);
}

void WebService::sendHtmlNumber(const char* label, long value, const char* unit) {
  char text[32];
  snprintf(text, sizeof(text), "%ld%s", value, unit);
  sendHtmlValue(label, text);
}

void WebService::sendHome() {
  server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server_.send(200, "text/html; charset=utf-8", "");
  server_.sendContent_P(kPageStart);
  sendHtmlValue("Projet", projectName_);
  sendHtmlValue("Firmware", firmwareVersion_);
  sendHtmlValue("Device ID", identity_->id());
  sendHtmlValue("Hostname", identity_->hostname());
  sendHtmlValue("Wi-Fi", wifi_->stateName());
  const String ssid = wifi_->currentSsid();
  sendHtmlValue("SSID", ssid.length() ? ssid.c_str() : "-");
  const String ip = wifi_->stationIp();
  const String apIp = wifi_->apIp();
  sendHtmlValue("Adresse IP", ip.length() ? ip.c_str() : apIp.c_str());
  sendHtmlNumber("RSSI", wifi_->rssi(), " dBm");
  sendHtmlNumber("Uptime", static_cast<long>(millis() / 1000UL), " s");
  sendHtmlNumber("Heap libre", PlatformCompat::freeHeap(), " octets");
  server_.sendContent_P(wifi_->apActive() ? kProvisioningForm : kPageEnd);
  server_.sendContent("");
}

void WebService::jsonEscape(const char* input, char* output, size_t capacity) {
  if (!output || capacity == 0) return;
  size_t written = 0;
  for (size_t index = 0; input && input[index] && written + 1 < capacity; ++index) {
    const char value = input[index];
    if ((value == '"' || value == '\\') && written + 2 < capacity) {
      output[written++] = '\\';
      output[written++] = value;
    } else if (static_cast<unsigned char>(value) >= 0x20) {
      output[written++] = value;
    }
  }
  output[written] = '\0';
}

void WebService::htmlEscape(const char* input, char* output, size_t capacity) {
  if (!output || capacity == 0) return;
  size_t written = 0;
  for (size_t index = 0; input && input[index] && written + 1 < capacity; ++index) {
    const char value = input[index];
    const char* replacement = nullptr;
    if (value == '&') replacement = "&amp;";
    else if (value == '<') replacement = "&lt;";
    else if (value == '>') replacement = "&gt;";
    else if (value == '"') replacement = "&quot;";
    else if (value == '\'') replacement = "&#39;";
    if (replacement) {
      const size_t length = strlen(replacement);
      if (written + length >= capacity) break;
      memcpy(output + written, replacement, length);
      written += length;
    } else if (static_cast<unsigned char>(value) >= 0x20) {
      output[written++] = value;
    }
  }
  output[written] = '\0';
}

void WebService::sendStatus() {
  char ssid[67];
  const String currentSsid = wifi_->currentSsid();
  jsonEscape(currentSsid.c_str(), ssid, sizeof(ssid));
  const String stationIp = wifi_->stationIp();
  const String fallbackIp = wifi_->apIp();
  char json[640];
  snprintf(json, sizeof(json),
           "{\"firmware\":\"%s\",\"version\":\"%s\",\"device_id\":\"%s\","
           "\"hostname\":\"%s\",\"uptime_ms\":%lu,\"free_heap\":%lu,"
           "\"minimum_heap\":%lu,\"web_heap_before\":%lu,\"web_heap_after\":%lu,"
           "\"wifi_state\":\"%s\",\"wifi_mode\":\"%s\",\"ssid\":\"%s\","
           "\"ip\":\"%s\",\"rssi\":%ld,\"ap_active\":%s,\"ap_ip\":\"%s\"}",
           projectName_, firmwareVersion_, identity_->id(),
           identity_->hostname(), static_cast<unsigned long>(millis()),
           static_cast<unsigned long>(PlatformCompat::freeHeap()),
           static_cast<unsigned long>(minimumHeap_), static_cast<unsigned long>(heapBeforeBegin_),
           static_cast<unsigned long>(heapAfterBegin_), wifi_->stateName(), wifi_->modeName(), ssid,
           stationIp.c_str(), static_cast<long>(wifi_->rssi()),
           wifi_->apActive() ? "true" : "false", fallbackIp.c_str());
  server_.send(200, "application/json", json);
}

void WebService::sendLogs() {
  server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server_.send(200, "text/plain; charset=utf-8", "");
  for (size_t index = 0; index < logs_->count(); ++index) {
    server_.sendContent(logs_->line(index));
    server_.sendContent("\n");
  }
  server_.sendContent("");
}

void WebService::saveWiFi() {
  if (!wifi_->apActive()) {
    server_.send(403, "text/plain", "Wi-Fi provisioning is available from fallback AP only\n");
    return;
  }
  if (!server_.hasArg("ssid") || !server_.hasArg("password")) {
    server_.send(400, "text/plain", "SSID and password are required\n");
    return;
  }
  const String ssid = server_.arg("ssid");
  const String password = server_.arg("password");
  if (!config_->saveWiFi(ssid.c_str(), password.c_str())) {
    server_.send(400, "text/plain", "Invalid Wi-Fi configuration\n");
    return;
  }
  server_.send(200, "text/html; charset=utf-8",
               "<!doctype html><meta charset='utf-8'><title>ESP_BASE</title>"
               "<p>Configuration enregistrée. Connexion en cours.</p>");
  logs_->add("CONFIG", "Wi-Fi configuration saved from Web for SSID %s", ssid.c_str());
  wifi_->credentialsChanged();
}

