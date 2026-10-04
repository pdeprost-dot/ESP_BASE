#include "WebService.h"

#include "PlatformCompat.h"

namespace {
const char kShellStart[] PROGMEM =
    "<!doctype html><html lang='fr'><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>ESP_BASE</title><style>*{box-sizing:border-box}:root{--bg:#eef3f7;"
    "--card:#fff;--ink:#17212b;--muted:#667580;--accent:#0874c9}body{margin:0;"
    "background:var(--bg);color:var(--ink);font:15px system-ui,sans-serif}"
    "header{background:#173149;color:#fff}.bar,main{max-width:960px;margin:auto;"
    "padding:14px 18px}.bar{display:flex;align-items:center;gap:18px}.brand{font-size:"
    "19px;font-weight:750;margin-right:auto}nav{display:flex;gap:4px;flex-wrap:wrap}"
    "nav a{color:#dce9f4;text-decoration:none;padding:9px;border-radius:7px}nav a.active{"
    "background:#ffffff20;color:#fff}main{padding-top:22px}.grid{display:grid;"
    "grid-template-columns:repeat(2,minmax(0,1fr));gap:14px}.card{background:var(--card);"
    "border-radius:12px;box-shadow:0 3px 14px #1b304012;padding:17px}.metrics{display:grid;"
    "grid-template-columns:minmax(8rem,1fr) auto;gap:9px 14px}.metrics b{text-align:right;"
    "overflow-wrap:anywhere}.hero{font-size:2rem;font-weight:750}.muted{color:var(--muted)}"
    "pre{white-space:pre-wrap;overflow:auto;max-height:28rem}input,button{font:inherit;"
    "padding:9px;border:1px solid #cbd6de;border-radius:7px}input{width:100%}label{display:"
    "block;margin:10px 0}.primary{background:var(--accent);color:#fff;border-color:"
    "var(--accent)}footer{max-width:960px;margin:auto;padding:12px 18px 24px;color:"
    "var(--muted)}@media(max-width:640px){.bar{align-items:flex-start;flex-direction:"
    "column}.brand{margin:0}.grid{grid-template-columns:1fr}nav{width:100%}nav a{"
    "padding:8px 7px}.metrics{grid-template-columns:1fr}.metrics b{text-align:left}}"
    "</style></head><body><header><div class='bar'><div class='brand'>";
const char kShellAfterBrand[] PROGMEM = "</div><nav>";
const char kShellAfterNav[] PROGMEM = "</nav></div></header><main>";
const char kShellEnd[] PROGMEM =
    "</main><footer>ESP_BASE &middot; interface locale embarquee</footer></body></html>";
const char kProvisioningForm[] PROGMEM =
    "<h2>Configuration Wi-Fi</h2>"
    "<form method='post' action='/api/wifi'>"
    "<p><label>SSID<br><input name='ssid' maxlength='32' required></label></p>"
    "<p><label>Mot de passe<br><input name='password' type='password' "
    "maxlength='64' autocomplete='new-password'></label></p>"
    "<p><button type='submit'>Enregistrer et connecter</button></p></form>"
    "<p class='muted'>Cette interface HTTP est réservée au réseau local de confiance.</p>";
}  // namespace

WebService::WebService() : server_(80) {}

bool WebService::addGetRoute(const char* path, WebRouteHandler handler, void* context) {
  return addApplicationRoute(nullptr, path, handler, context);
}

bool WebService::addPage(const char* label, const char* path,
                         WebRouteHandler handler, void* context) {
  if (!label || label[0] == '\0' || strlen(label) > kMaxPageLabelLength) return false;
  return addApplicationRoute(label, path, handler, context);
}

bool WebService::addApplicationRoute(const char* label, const char* path,
                                     WebRouteHandler handler, void* context) {
  if (started_ || !path || path[0] != '/' || path[1] == '\0' || !handler ||
      strlen(path) > kMaxRoutePathLength || isReservedRoute(path) ||
      applicationRouteCount_ >= kMaxApplicationRoutes) {
    return false;
  }
  for (size_t index = 0; index < applicationRouteCount_; ++index) {
    if (strcmp(applicationRoutes_[index].path, path) == 0) return false;
  }
  ApplicationRoute& route = applicationRoutes_[applicationRouteCount_++];
  strlcpy(route.path, path, sizeof(route.path));
  if (label) strlcpy(route.label, label, sizeof(route.label));
  route.handler = handler;
  route.context = context;
  return true;
}

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
  started_ = true;
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
  server_.on("/wifi", HTTP_GET, [this]() { sendWifiPage(); });
  server_.on("/logs", HTTP_GET, [this]() { sendLogsPage(); });
  server_.on("/system", HTTP_GET, [this]() { sendSystemPage(); });
  server_.on("/api/status", HTTP_GET, [this]() { sendStatus(); });
  server_.on("/api/logs", HTTP_GET, [this]() { sendLogs(); });
  server_.on("/api/wifi", HTTP_POST, [this]() { saveWiFi(); });
  for (size_t index = 0; index < applicationRouteCount_; ++index) {
    server_.on(applicationRoutes_[index].path, HTTP_GET,
               [this, index]() { dispatchApplicationRoute(index); });
  }
  server_.onNotFound([this]() { server_.send(404, "text/plain", "Not found\n"); });
}

bool WebService::isReservedRoute(const char* path) {
  return strcmp(path, "/") == 0 || strcmp(path, "/api/status") == 0 ||
         strcmp(path, "/api/logs") == 0 || strcmp(path, "/api/wifi") == 0 ||
         strcmp(path, "/wifi") == 0 || strcmp(path, "/logs") == 0 ||
         strcmp(path, "/system") == 0;
}

void WebService::sendApplicationResponse(void* context, uint16_t statusCode,
                                         const char* contentType, const char* body) {
  static_cast<WebService*>(context)->server_.send(statusCode, contentType, body);
}

void WebService::beginApplicationPage(void* context, const char* title) {
  WebService* service = static_cast<WebService*>(context);
  service->beginPage(title, service->activeApplicationPath_);
}

void WebService::writeApplicationPage(void* context, const char* html) {
  static_cast<WebService*>(context)->server_.sendContent(html);
}

void WebService::endApplicationPage(void* context) {
  static_cast<WebService*>(context)->endPage();
}

void WebService::dispatchApplicationRoute(size_t index) {
  if (index >= applicationRouteCount_) {
    server_.send(500, "text/plain", "Invalid route\n");
    return;
  }
  ApplicationRoute& route = applicationRoutes_[index];
  activeApplicationPath_ = route.path;
  WebResponse response(sendApplicationResponse, beginApplicationPage,
                       writeApplicationPage, endApplicationPage, this);
  route.handler(response, route.context);
  activeApplicationPath_ = nullptr;
  if (!response.sent()) server_.send(500, "text/plain", "Handler did not respond\n");
}

void WebService::sendHtmlValue(const char* label, const char* value) {
  char escaped[129];
  htmlEscape(value, escaped, sizeof(escaped));
  char row[192];
  snprintf(row, sizeof(row), "<span>%s</span><b>%s</b>", label, escaped);
  server_.sendContent(row);
}

void WebService::sendHtmlNumber(const char* label, long value, const char* unit) {
  char text[32];
  snprintf(text, sizeof(text), "%ld%s", value, unit);
  sendHtmlValue(label, text);
}

void WebService::sendNavItem(const char* label, const char* path, const char* activePath) {
  char escapedLabel[64];
  htmlEscape(label, escapedLabel, sizeof(escapedLabel));
  char item[160];
  snprintf(item, sizeof(item), "<a%s href='%s'>%s</a>",
           strcmp(path, activePath) == 0 ? " class='active'" : "", path, escapedLabel);
  server_.sendContent(item);
}

void WebService::beginPage(const char* title, const char* activePath) {
  server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server_.send(200, "text/html; charset=utf-8", "");
  server_.sendContent_P(kShellStart);
  char escapedProject[67];
  htmlEscape(projectName_, escapedProject, sizeof(escapedProject));
  server_.sendContent(escapedProject);
  server_.sendContent_P(kShellAfterBrand);
  sendNavItem("Accueil", "/", activePath);
  for (size_t index = 0; index < applicationRouteCount_; ++index) {
    if (applicationRoutes_[index].label[0] != '\0') {
      sendNavItem(applicationRoutes_[index].label, applicationRoutes_[index].path, activePath);
    }
  }
  sendNavItem("Wi-Fi", "/wifi", activePath);
  sendNavItem("Logs", "/logs", activePath);
  sendNavItem("Système", "/system", activePath);
  server_.sendContent_P(kShellAfterNav);
  char escapedTitle[67];
  htmlEscape(title, escapedTitle, sizeof(escapedTitle));
  char heading[96];
  snprintf(heading, sizeof(heading), "<h1>%s</h1>", escapedTitle);
  server_.sendContent(heading);
}

void WebService::endPage() {
  server_.sendContent_P(kShellEnd);
  server_.sendContent("");
}

void WebService::sendHome() {
  beginPage("Accueil", "/");
  server_.sendContent("<div class='grid'><section class='card'><h2>Appareil</h2>"
                      "<div class='metrics'>");
  sendHtmlValue("Projet", projectName_);
  sendHtmlValue("Firmware", firmwareVersion_);
  sendHtmlValue("Device ID", identity_->id());
  sendHtmlValue("Hostname", identity_->hostname());
  server_.sendContent("</div></section><section class='card'><h2>État</h2>"
                      "<div class='metrics'>");
  sendHtmlValue("Wi-Fi", wifi_->stateName());
  const String ssid = wifi_->currentSsid();
  sendHtmlValue("SSID", ssid.length() ? ssid.c_str() : "-");
  const String ip = wifi_->stationIp();
  const String apIp = wifi_->apIp();
  sendHtmlValue("Adresse IP", ip.length() ? ip.c_str() : apIp.c_str());
  sendHtmlNumber("RSSI", wifi_->rssi(), " dBm");
  sendHtmlNumber("Uptime", static_cast<long>(millis() / 1000UL), " s");
  sendHtmlNumber("Heap libre", PlatformCompat::freeHeap(), " octets");
  server_.sendContent("</div></section></div>");
  endPage();
}

void WebService::sendWifiPage() {
  beginPage("Wi-Fi", "/wifi");
  server_.sendContent("<section class='card'><div class='metrics'>");
  sendHtmlValue("État", wifi_->stateName());
  const String ssid = wifi_->currentSsid();
  sendHtmlValue("SSID", ssid.length() ? ssid.c_str() : "-");
  const String stationIp = wifi_->stationIp();
  const String fallbackIp = wifi_->apIp();
  sendHtmlValue("Adresse IP", stationIp.length() ? stationIp.c_str() : fallbackIp.c_str());
  sendHtmlNumber("RSSI", wifi_->rssi(), " dBm");
  sendHtmlValue("Hostname", identity_->hostname());
  sendHtmlValue("Mode", wifi_->modeName());
  server_.sendContent("</div></section>");
  if (wifi_->apActive()) {
    server_.sendContent_P(kProvisioningForm);
  } else {
    server_.sendContent("<p class='muted'>La reconfiguration est volontairement indisponible "
                        "depuis le STA. Utilisez la commande Serial CLEAR pour revenir au mode "
                        "de provisioning sécurisé.</p>");
  }
  endPage();
}

void WebService::sendLogsPage() {
  beginPage("Logs", "/logs");
  server_.sendContent("<section class='card'><pre>");
  for (size_t index = 0; index < logs_->count(); ++index) {
    char escaped[192];
    htmlEscape(logs_->line(index), escaped, sizeof(escaped));
    server_.sendContent(escaped);
    server_.sendContent("\n");
  }
  server_.sendContent("</pre><p><a href='/api/logs'>Version texte brute</a></p></section>");
  endPage();
}

void WebService::sendSystemPage() {
  beginPage("Système", "/system");
  server_.sendContent("<section class='card'><div class='metrics'>");
  sendHtmlValue("Projet", projectName_);
  sendHtmlValue("Firmware", firmwareVersion_);
  sendHtmlValue("Device ID", identity_->id());
  sendHtmlValue("Hostname", identity_->hostname());
  sendHtmlNumber("Uptime", static_cast<long>(millis() / 1000UL), " s");
  sendHtmlNumber("Heap libre", PlatformCompat::freeHeap(), " octets");
  sendHtmlNumber("Heap minimum", minimumHeap_, " octets");
  sendHtmlNumber("Heap avant Web", heapBeforeBegin_, " octets");
  sendHtmlNumber("Heap après Web", heapAfterBegin_, " octets");
  server_.sendContent("</div><p><a href='/api/status'>Diagnostic JSON</a></p></section>");
  endPage();
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

