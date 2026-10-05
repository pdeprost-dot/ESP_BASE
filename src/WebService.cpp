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
}  // namespace

WebService::WebService() : server_(80) {}

bool WebService::addGetRoute(const char* path, WebRouteHandler handler, void* context) {
  return addApplicationRoute(nullptr, path, handler, nullptr, context, false);
}

bool WebService::addGetRoute(const char* path, WebRequestRouteHandler handler,
                             void* context) {
  return addApplicationRoute(nullptr, path, nullptr, handler, context, false);
}

bool WebService::addPostRoute(const char* path, WebRequestRouteHandler handler,
                              void* context) {
  return addApplicationRoute(nullptr, path, nullptr, handler, context, true);
}

bool WebService::addPage(const char* label, const char* path,
                         WebRouteHandler handler, void* context) {
  if (!label || label[0] == '\0' || strlen(label) > kMaxPageLabelLength) return false;
  return addApplicationRoute(label, path, handler, nullptr, context, false);
}

bool WebService::addPage(const char* label, const char* path,
                         WebRequestRouteHandler handler, void* context) {
  if (!label || label[0] == '\0' || strlen(label) > kMaxPageLabelLength) return false;
  return addApplicationRoute(label, path, nullptr, handler, context, false);
}

bool WebService::addApplicationRoute(const char* label, const char* path,
                                     WebRouteHandler legacyHandler,
                                     WebRequestRouteHandler requestHandler, void* context,
                                     bool postRoute) {
  if (started_ || !path || path[0] != '/' || path[1] == '\0' ||
      (!legacyHandler && !requestHandler) ||
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
  route.requestAware = requestHandler != nullptr;
  if (route.requestAware) route.handler.request = requestHandler;
  else route.handler.legacy = legacyHandler;
  route.context = context;
  route.postRoute = postRoute;
  return true;
}

void WebService::begin(const char* projectName, const char* firmwareVersion,
                       const DeviceIdentity& identity, ConfigStore& config,
                       WiFiService& wifi, OtaService& ota, MqttService& mqtt,
                       LogService& logs) {
  projectName_ = projectName;
  firmwareVersion_ = firmwareVersion;
  identity_ = &identity;
  config_ = &config;
  wifi_ = &wifi;
  ota_ = &ota;
  mqtt_ = &mqtt;
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
  if (wifiChangePending_ && static_cast<int32_t>(millis() - wifiChangeAt_) >= 0) {
    wifiChangePending_ = false;
    wifi_->credentialsChanged();
  }
  if (apPasswordChangePending_ &&
      static_cast<int32_t>(millis() - apPasswordChangeAt_) >= 0) {
    apPasswordChangePending_ = false;
    wifi_->apSettingsChanged(apPasswordChangedPending_);
    apPasswordChangedPending_ = false;
  }
  const uint32_t current = PlatformCompat::freeHeap();
  if (current < minimumHeap_) minimumHeap_ = current;
}

void WebService::registerRoutes() {
  server_.on("/", HTTP_GET, [this]() { sendHome(); });
  server_.on("/setup", HTTP_GET, [this]() { sendSetupPage(); });
  server_.on("/wifi", HTTP_GET, [this]() { sendWifiPage(); });
  server_.on("/logs", HTTP_GET, [this]() { sendLogsPage(); });
  server_.on("/ota", HTTP_GET, [this]() { sendOtaPage(); });
  server_.on("/mqtt", HTTP_GET, [this]() { sendMqttPage(); });
  server_.on("/system", HTTP_GET, [this]() { sendSystemPage(); });
  server_.on("/api/status", HTTP_GET, [this]() { sendStatus(); });
  server_.on("/api/logs", HTTP_GET, [this]() { sendLogs(); });
  server_.on("/api/wifi", HTTP_POST, [this]() { saveWiFi(); });
  server_.on("/api/wifi/setup", HTTP_POST, [this]() { saveWifiSetup(); });
  server_.on("/api/wifi/config", HTTP_POST, [this]() { saveWifiConfig(); });
  server_.on("/api/wifi/scan", HTTP_POST, [this]() { startWifiScan(); });
  server_.on("/api/wifi/scan", HTTP_GET, [this]() { sendWifiScan(); });
  server_.on("/api/ap-password", HTTP_POST, [this]() { saveApPassword(); });
  server_.on("/api/mqtt", HTTP_POST, [this]() { saveMqtt(); });
  server_.on("/api/ota", HTTP_POST, [this]() { finishOtaUpload(); },
             [this]() { handleOtaUpload(); });
  for (size_t index = 0; index < applicationRouteCount_; ++index) {
    server_.on(applicationRoutes_[index].path,
               applicationRoutes_[index].postRoute ? HTTP_POST : HTTP_GET,
               [this, index]() { dispatchApplicationRoute(index); });
  }
  server_.onNotFound([this]() { server_.send(404, "text/plain", "Not found\n"); });
}

bool WebService::isReservedRoute(const char* path) {
  return strcmp(path, "/") == 0 || strcmp(path, "/api/status") == 0 ||
         strcmp(path, "/api/logs") == 0 || strcmp(path, "/api/wifi") == 0 ||
         strcmp(path, "/api/ap-password") == 0 ||
         strcmp(path, "/api/mqtt") == 0 ||
         strcmp(path, "/api/wifi/config") == 0 || strcmp(path, "/api/wifi/setup") == 0 ||
         strcmp(path, "/api/wifi/scan") == 0 ||
         strcmp(path, "/setup") == 0 ||
         strcmp(path, "/wifi") == 0 || strcmp(path, "/logs") == 0 ||
         strcmp(path, "/ota") == 0 || strcmp(path, "/system") == 0 ||
         strcmp(path, "/mqtt") == 0 ||
         strcmp(path, "/api/ota") == 0;
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

bool WebService::applicationRequestHasArg(void* context, const char* name) {
  return static_cast<WebService*>(context)->server_.hasArg(name);
}

String WebService::applicationRequestArg(void* context, const char* name) {
  return static_cast<WebService*>(context)->server_.arg(name);
}

void WebService::dispatchApplicationRoute(size_t index) {
  if (index >= applicationRouteCount_) {
    server_.send(500, "text/plain", "Invalid route\n");
    return;
  }
  ApplicationRoute& route = applicationRoutes_[index];
  activeApplicationPath_ = route.path;
  WebRequest request(applicationRequestHasArg, applicationRequestArg, this);
  WebResponse response(sendApplicationResponse, beginApplicationPage,
                       writeApplicationPage, endApplicationPage, this);
  if (route.requestAware) route.handler.request(request, response, route.context);
  else route.handler.legacy(response, route.context);
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
  sendNavItem("MQTT", "/mqtt", activePath);
  if (wifi_->apActive()) sendNavItem("Provisioning", "/setup", activePath);
  sendNavItem("Logs", "/logs", activePath);
  if (ota_->enabled()) sendNavItem("OTA", "/ota", activePath);
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
  server_.sendContent("<div class='grid'><section class='card'><h2>État réseau</h2>"
                      "<div class='metrics'>");
  sendHtmlValue("État", wifi_->stateName());
  const String ssid = wifi_->currentSsid();
  sendHtmlValue("SSID", ssid.length() ? ssid.c_str() : "-");
  const String stationIp = wifi_->stationIp();
  const String fallbackIp = wifi_->apIp();
  sendHtmlValue("Adresse IP", stationIp.length() ? stationIp.c_str() : fallbackIp.c_str());
  sendHtmlNumber("RSSI", wifi_->rssi(), " dBm");
  sendHtmlValue("Hostname", identity_->hostname());
  sendHtmlValue("Mode", wifi_->modeName());
  sendHtmlNumber("STA actif", wifi_->connectedSlot() >= 0 ? wifi_->connectedSlot() + 1 : 0);
  server_.sendContent("</div></section></div>");

  {
    char ssid1[67], pass1[129], ssid2[67], pass2[129], apPass[129];
    htmlEscape(config_->ssid(0), ssid1, sizeof(ssid1));
    htmlEscape(config_->password(0), pass1, sizeof(pass1));
    htmlEscape(config_->ssid(1), ssid2, sizeof(ssid2));
    htmlEscape(config_->password(1), pass2, sizeof(pass2));
    htmlEscape(config_->apPassword(), apPass, sizeof(apPass));
    char form[1280];
    snprintf(form, sizeof(form),
             "<form method='post' action='/api/wifi/config'><div class='grid'>"
             "<section class='card'><h2>Réseau principal — STA1</h2>"
             "<label>SSID<input id='sta1' name='sta1_ssid' maxlength='32' value='%s' required></label>"
             "<label>Mot de passe<input id='p1' name='sta1_password' type='password' maxlength='63' value='%s'></label>"
             "<button type='button' onclick=\"toggleSecret('p1',this)\">Afficher</button></section>"
             "<section class='card'><h2>Réseau secondaire — STA2</h2>"
             "<label>SSID<input id='sta2' name='sta2_ssid' maxlength='32' value='%s'></label>"
             "<label>Mot de passe<input id='p2' name='sta2_password' type='password' maxlength='63' value='%s'></label>"
             "<button type='button' onclick=\"toggleSecret('p2',this)\">Afficher</button></section></div>"
             "<section class='card'><button class='primary' type='submit'>Enregistrer STA1 et STA2</button></section></form>",
             ssid1, pass1, ssid2, pass2);
    server_.sendContent(form);
    server_.sendContent("<section class='card'><h2>Point d'accès ESP_BASE</h2>"
                        "<div class='metrics'>");
    sendHtmlValue("État AP", wifi_->apActive() ? "ACTIF" : "INACTIF");
    sendHtmlValue("SSID AP", wifi_->apSsid());
    sendHtmlValue("Adresse IP AP", fallbackIp.length() ? fallbackIp.c_str() : "-");
    sendHtmlValue("Mode AP permanent", config_->apAlwaysOn() ? "ACTIVÉ" : "DÉSACTIVÉ");
    server_.sendContent("</div><form method='post' action='/api/ap-password'>");
    snprintf(form, sizeof(form),
             "<label>Mot de passe AP<input id='pap' name='ap_password' type='password' "
             "minlength='8' maxlength='63' value='%s' required></label>"
             "<button type='button' onclick=\"toggleSecret('pap',this)\">Afficher</button>"
             "<p class='muted'>SSID automatique propre à l'appareil. "
             "Mot de passe usine : ESPbaseSetup.</p>"
             "<label><input name='ap_always_on' type='checkbox' value='1' style='width:auto'%s> "
             "Maintenir le point d'accès actif même lorsqu'un réseau LAN est connecté</label>"
             "<p class='muted'>Désactivé : l'AP s'active automatiquement uniquement si STA1 et STA2 "
             "sont indisponibles.<br>Activé : l'AP reste accessible même lorsqu'un STA est connecté.</p>"
             "<button class='primary' type='submit'>Enregistrer l'AP</button> "
             "<button name='reset' value='1' type='submit' formnovalidate>Rétablir le mot de passe AP par défaut</button>"
             "</form></section>", apPass, config_->apAlwaysOn() ? " checked" : "");
    server_.sendContent(form);
  }

  sendScanPanel();
  endPage();
}

void WebService::sendSetupPage() {
  if (!wifi_->apActive()) {
    server_.send(409, "text/plain", "Provisioning is available only from the fallback AP\n");
    return;
  }
  beginPage("Provisioning Wi-Fi", "/setup");
  server_.sendContent("<section class='card'><p>Configurez un réseau principal et, si souhaité, "
                      "un réseau de secours. Aucun credential existant n'est affiché ici.</p></section>");
  sendWifiForm();
  sendScanPanel();
  endPage();
}

void WebService::sendScanPanel() {
  server_.sendContent("<section class='card'><h2>Réseaux disponibles</h2>"
                      "<button type='button' onclick='startScan()'>Rechercher les réseaux</button>"
                      "<div id='scan' class='metrics'><span>Scan</span><b>non lancé</b></div></section>"
                      "<script>function toggleSecret(id,b){var e=document.getElementById(id);"
                      "e.type=e.type==='password'?'text':'password';b.textContent=e.type==='password'?'Afficher':'Masquer'}"
                      "function useSsid(id,s){var e=document.getElementById(id);if(e)e.value=s}"
                      "function showScan(j){var d=document.getElementById('scan');d.innerHTML='';"
                      "if(j.state==='RUNNING'){d.textContent='Scan en cours…';setTimeout(loadScan,600);return}"
                      "j.networks.forEach(function(n){var t=document.createElement('span');"
                      "t.textContent=n.ssid+' · '+n.rssi+' dBm · '+(n.encrypted?'sécurisé':'ouvert');"
                      "var b=document.createElement('b'),a=document.createElement('button'),c=document.createElement('button');"
                      "a.textContent='STA1';c.textContent='STA2';a.onclick=function(){useSsid('sta1',n.ssid)};"
                      "c.onclick=function(){useSsid('sta2',n.ssid)};b.append(a,c);d.append(t,b)})}"
                      "function loadScan(){fetch('/api/wifi/scan').then(r=>r.json()).then(showScan)}"
                      "function startScan(){fetch('/api/wifi/scan',{method:'POST'}).then(loadScan)}</script>");
}

void WebService::sendWifiForm() {
  server_.sendContent(
      "<form method='post' action='/api/wifi/setup'><div class='grid'>"
      "<section class='card'><h2>Réseau principal — STA1</h2>"
      "<label>SSID<input id='sta1' name='sta1_ssid' maxlength='32' required></label>"
      "<label>Mot de passe<input id='p1' name='sta1_password' type='password' maxlength='63'></label>"
      "<button type='button' onclick=\"toggleSecret('p1',this)\">Afficher</button></section>"
      "<section class='card'><h2>Réseau secondaire — STA2</h2>"
      "<label>SSID<input id='sta2' name='sta2_ssid' maxlength='32'></label>"
      "<label>Mot de passe<input id='p2' name='sta2_password' type='password' maxlength='63'></label>"
      "<button type='button' onclick=\"toggleSecret('p2',this)\">Afficher</button></section></div>"
      "<section class='card'><p class='muted'>Les SSID restent librement éditables pour les "
      "réseaux masqués ou absents du scan. Laisser STA2 entièrement vide pour le désactiver.</p>"
      "<button class='primary' type='submit'>Enregistrer et connecter</button></section></form>");
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

void WebService::sendOtaPage() {
  beginPage("Mise à jour OTA", "/ota");
  server_.sendContent("<section class='card'><div class='metrics'>");
  sendHtmlValue("Firmware", firmwareVersion_);
  sendHtmlValue("Device ID", identity_->id());
  sendHtmlValue("ArduinoOTA", wifi_->connected() ? "PRÊT SUR LE LAN" : "INDISPONIBLE");
  server_.sendContent(
      "</div><h2>Firmware Web OTA</h2><p class='muted'>Sélectionnez uniquement un fichier "
      ".bin compilé pour cette carte. La configuration Wi-Fi est conservée.</p>"
      "<form id='otaForm' enctype='multipart/form-data'><input name='firmware' type='file' "
      "accept='.bin' required><p><button class='primary'>Installer</button></p>"
      "<progress id='otaProgress' max='100' value='0' style='width:100%'></progress> "
      "<span id='otaResult'></span></form></section>"
      "<script>otaForm.onsubmit=function(e){e.preventDefault();var x=new XMLHttpRequest(),"
      "f=new FormData(otaForm);x.open('POST','/api/ota');x.upload.onprogress=function(p){"
      "if(p.lengthComputable)otaProgress.value=p.loaded*100/p.total};x.onload=function(){"
      "otaResult.textContent=x.status===200?'Mise à jour réussie, redémarrage…':"
      "'Échec: '+x.responseText};x.onerror=function(){otaResult.textContent="
      "'Connexion interrompue'};x.send(f)};</script>");
  endPage();
}

void WebService::sendMqttPage() {
  beginPage("MQTT", "/mqtt");
  server_.sendContent("<div class='grid'><section class='card'><h2>Etat</h2><div class='metrics'>");
  sendHtmlValue("Service", config_->mqttEnabled() ? "ACTIVE" : "DESACTIVE");
  sendHtmlValue("Connexion", mqtt_->connected() ? "CONNECTE" : "DECONNECTE");
  sendHtmlValue("Client ID", mqtt_->clientId());
  sendHtmlNumber("Derniere erreur", mqtt_->lastError());
  sendHtmlNumber("Tentatives", mqtt_->connectionAttempts());
  sendHtmlNumber("Reconnexions", mqtt_->reconnects());
  sendHtmlNumber("Publications", mqtt_->publications());
  sendHtmlNumber("Echecs publication", mqtt_->publishFailures());
  sendHtmlNumber("Messages recus", mqtt_->messagesReceived());
  server_.sendContent("</div></section></div>");
  char broker[129], username[81], password[129], root[129];
  htmlEscape(config_->mqttBroker(), broker, sizeof(broker));
  htmlEscape(config_->mqttUsername(), username, sizeof(username));
  htmlEscape(config_->mqttPassword(), password, sizeof(password));
  htmlEscape(config_->mqttRootTopic(), root, sizeof(root));
  char form[640];
  snprintf(form, sizeof(form),
           "<section class='card'><h2>Configuration</h2><form method='post' action='/api/mqtt'>"
           "<label><input name='enabled' type='checkbox' value='1' style='width:auto'%s> MQTT active</label>"
           "<label>Broker hostname/IP<input name='broker' maxlength='63' value='%s'></label>"
           "<label>Port<input name='port' type='number' min='1' max='65535' value='%u' required></label>",
           config_->mqttEnabled() ? " checked" : "", broker, config_->mqttPort());
  server_.sendContent(form);
  snprintf(form, sizeof(form),
           "<label>Utilisateur<input name='username' maxlength='39' value='%s'></label>"
           "<label>Mot de passe<input id='mqttPassword' name='password' type='password' maxlength='63' value='%s'></label>",
           username, password);
  server_.sendContent(form);
  snprintf(form, sizeof(form),
           "<button type='button' onclick=\"toggleMqttSecret(this)\">Afficher</button>"
           "<label>Topic racine<input name='root_topic' maxlength='64' value='%s' required></label>"
           "<p class='muted'>Les topics applicatifs sont relatifs a cette racine. L'interface locale n'est pas authentifiee : le secret MQTT est accessible aux utilisateurs du LAN.</p>"
           "<button class='primary' type='submit'>Enregistrer</button></form></section>", root);
  server_.sendContent(form);
  server_.sendContent("<script>function toggleMqttSecret(b){var e=document.getElementById('mqttPassword');"
                      "e.type=e.type==='password'?'text':'password';"
                      "b.textContent=e.type==='password'?'Afficher':'Masquer'}</script>");
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
  sendHtmlNumber("Heap minimum OTA", ota_->minimumHeap(), " octets");
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
  char mqttBroker[129], mqttRoot[129], mqttClientId[97];
  jsonEscape(config_->mqttBroker(), mqttBroker, sizeof(mqttBroker));
  jsonEscape(config_->mqttRootTopic(), mqttRoot, sizeof(mqttRoot));
  jsonEscape(mqtt_->clientId(), mqttClientId, sizeof(mqttClientId));
  char json[1088];
  snprintf(json, sizeof(json),
           "{\"firmware\":\"%s\",\"version\":\"%s\",\"device_id\":\"%s\","
           "\"hostname\":\"%s\",\"uptime_ms\":%lu,\"free_heap\":%lu,"
           "\"minimum_heap\":%lu,\"web_heap_before\":%lu,\"web_heap_after\":%lu,"
           "\"wifi_state\":\"%s\",\"wifi_mode\":\"%s\",\"ssid\":\"%s\","
           "\"ip\":\"%s\",\"rssi\":%ld,\"sta_slot\":%d,\"ap_active\":%s,"
           "\"ap_always_on\":%s,\"ap_ip\":\"%s\","
           "\"mqtt_enabled\":%s,\"mqtt_connected\":%s,\"mqtt_broker\":\"%s\","
           "\"mqtt_port\":%u,\"mqtt_root_topic\":\"%s\",\"mqtt_client_id\":\"%s\","
           "\"mqtt_last_error\":%d,\"mqtt_attempts\":%lu,\"mqtt_reconnections\":%lu,"
           "\"mqtt_publications\":%lu,\"mqtt_publish_failures\":%lu,\"mqtt_messages_received\":%lu}",
           projectName_, firmwareVersion_, identity_->id(),
           identity_->hostname(), static_cast<unsigned long>(millis()),
           static_cast<unsigned long>(PlatformCompat::freeHeap()),
           static_cast<unsigned long>(minimumHeap_), static_cast<unsigned long>(heapBeforeBegin_),
           static_cast<unsigned long>(heapAfterBegin_), wifi_->stateName(), wifi_->modeName(), ssid,
           stationIp.c_str(), static_cast<long>(wifi_->rssi()), wifi_->connectedSlot() + 1,
           wifi_->apActive() ? "true" : "false",
           config_->apAlwaysOn() ? "true" : "false", fallbackIp.c_str(),
           config_->mqttEnabled() ? "true" : "false", mqtt_->connected() ? "true" : "false",
           mqttBroker, config_->mqttPort(), mqttRoot, mqttClientId, mqtt_->lastError(),
           static_cast<unsigned long>(mqtt_->connectionAttempts()),
           static_cast<unsigned long>(mqtt_->reconnects()),
           static_cast<unsigned long>(mqtt_->publications()),
           static_cast<unsigned long>(mqtt_->publishFailures()),
           static_cast<unsigned long>(mqtt_->messagesReceived()));
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
  if (!server_.hasArg("ssid") || !server_.hasArg("password")) {
    server_.send(400, "text/plain", "SSID and password are required\n");
    return;
  }
  const String ssid = server_.arg("ssid");
  String password = server_.arg("password");
  const bool openNetwork = server_.hasArg("open_network") &&
                           server_.arg("open_network") == "1";
  if (openNetwork) {
    password = "";
  } else if (password.length() == 0) {
    if (!config_->configured() || ssid != config_->ssid()) {
      server_.send(400, "text/plain",
                   "Password required for a new SSID, or select open network\n");
      return;
    }
    password = config_->password();
  }
  if (!config_->saveWiFi(ssid.c_str(), password.c_str())) {
    server_.send(400, "text/plain", "Invalid Wi-Fi configuration\n");
    return;
  }
  server_.send(200, "text/html; charset=utf-8",
               "<!doctype html><meta charset='utf-8'><title>ESP_BASE</title>"
               "<p>Configuration enregistrée. Connexion en cours.</p>");
  logs_->add("CONFIG", "Wi-Fi configuration saved from Web for SSID %s", ssid.c_str());
  wifiChangePending_ = true;
  wifiChangeAt_ = millis() + 750U;
}

void WebService::saveWifiConfig() {
  if (!server_.hasArg("sta1_ssid") || !server_.hasArg("sta1_password") ||
      !server_.hasArg("sta2_ssid") || !server_.hasArg("sta2_password")) {
    server_.send(400, "text/plain", "All station fields are required\n");
    return;
  }
  if (!config_->saveStations(server_.arg("sta1_ssid").c_str(),
                             server_.arg("sta1_password").c_str(),
                             server_.arg("sta2_ssid").c_str(),
                             server_.arg("sta2_password").c_str())) {
    server_.send(400, "text/plain", "Invalid STA1/STA2 configuration\n");
    return;
  }
  server_.send(200, "text/html; charset=utf-8",
               "<!doctype html><meta charset='utf-8'><title>ESP_BASE</title>"
               "<p>STA1 et STA2 enregistrés. Reconnexion en cours.</p>");
  logs_->add("CONFIG", "STA1/STA2 configuration saved from Web UI");
  wifiChangePending_ = true;
  wifiChangeAt_ = millis() + 750U;
}

void WebService::saveWifiSetup() {
  if (!wifi_->apActive()) {
    server_.send(403, "text/plain", "Provisioning is available only from the fallback AP\n");
    return;
  }
  if (!server_.hasArg("sta1_ssid") || !server_.hasArg("sta1_password") ||
      !server_.hasArg("sta2_ssid") || !server_.hasArg("sta2_password")) {
    server_.send(400, "text/plain", "All station fields are required\n");
    return;
  }
  if (!config_->saveStations(server_.arg("sta1_ssid").c_str(),
                             server_.arg("sta1_password").c_str(),
                             server_.arg("sta2_ssid").c_str(),
                             server_.arg("sta2_password").c_str())) {
    server_.send(400, "text/plain", "Invalid STA1/STA2 configuration\n");
    return;
  }
  server_.send(200, "text/html; charset=utf-8",
               "<!doctype html><meta charset='utf-8'><title>ESP_BASE</title>"
               "<p>STA1 et STA2 enregistrés. Connexion en cours.</p>");
  logs_->add("CONFIG", "STA1/STA2 configuration saved from fallback provisioning");
  wifiChangePending_ = true;
  wifiChangeAt_ = millis() + 750U;
}

void WebService::saveApPassword() {
  const bool reset = server_.hasArg("reset") && server_.arg("reset") == "1";
  if (!reset && !server_.hasArg("ap_password")) {
    server_.send(400, "text/plain", "AP password is required\n");
    return;
  }
  const bool alwaysOn = server_.hasArg("ap_always_on") && server_.arg("ap_always_on") == "1";
  const String candidate = reset ? String(ConfigStore::defaultApPassword())
                                 : server_.arg("ap_password");
  const bool passwordChanged = candidate != config_->apPassword();
  const bool alwaysOnChanged = alwaysOn != config_->apAlwaysOn();
  const bool saved = config_->saveApSettings(candidate.c_str(), alwaysOn);
  if (!saved) {
    server_.send(400, "text/plain", "Invalid AP password (8-63 printable ASCII characters)\n");
    return;
  }
  server_.send(200, "text/html; charset=utf-8",
               "<!doctype html><meta charset='utf-8'><title>ESP_BASE</title>"
               "<p>Configuration AP enregistrée.</p>");
  if (passwordChanged) {
    logs_->add("CONFIG", reset ? "AP password restored to default"
                                : "AP password changed");
  }
  if (alwaysOnChanged) {
    logs_->add("CONFIG", alwaysOn ? "AP always-on enabled" : "AP always-on disabled");
  }
  if (passwordChanged || alwaysOnChanged) {
    apPasswordChangedPending_ = apPasswordChangedPending_ || passwordChanged;
    apPasswordChangePending_ = true;
    apPasswordChangeAt_ = millis() + 750U;
  }
}

void WebService::saveMqtt() {
  if (!server_.hasArg("broker") || !server_.hasArg("port") ||
      !server_.hasArg("username") || !server_.hasArg("password") ||
      !server_.hasArg("root_topic")) {
    server_.send(400, "text/plain", "All MQTT fields are required\n");
    return;
  }
  const long port = server_.arg("port").toInt();
  const bool enabled = server_.hasArg("enabled") && server_.arg("enabled") == "1";
  if (port < 1 || port > 65535 ||
      !config_->saveMqtt(enabled, server_.arg("broker").c_str(),
                         static_cast<uint16_t>(port), server_.arg("username").c_str(),
                         server_.arg("password").c_str(), server_.arg("root_topic").c_str())) {
    server_.send(400, "text/plain", "Invalid MQTT configuration\n");
    return;
  }
  mqtt_->configurationChanged();
  logs_->add("MQTT", "Configuration saved enabled=%s broker=%s port=%u root=%s",
             enabled ? "yes" : "no", config_->mqttBroker(), config_->mqttPort(),
             config_->mqttRootTopic());
  server_.sendHeader("Location", "/mqtt");
  server_.send(303, "text/plain", "Saved\n");
}

void WebService::startWifiScan() {
  if (!wifi_->startScan()) {
    server_.send(409, "application/json", "{\"started\":false}");
    return;
  }
  server_.send(202, "application/json", "{\"started\":true}");
}

void WebService::sendWifiScan() {
  server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server_.send(200, "application/json", "");
  char header[48];
  snprintf(header, sizeof(header), "{\"state\":\"%s\",\"networks\":[",
           wifi_->scanStateName());
  server_.sendContent(header);
  for (size_t index = 0; index < wifi_->scanCount(); ++index) {
    const WiFiService::ScanResult& result = wifi_->scanResult(index);
    char escaped[67];
    jsonEscape(result.ssid, escaped, sizeof(escaped));
    char item[128];
    snprintf(item, sizeof(item), "%s{\"ssid\":\"%s\",\"rssi\":%ld,\"encrypted\":%s}",
             index ? "," : "", escaped, static_cast<long>(result.rssi),
             result.encrypted ? "true" : "false");
    server_.sendContent(item);
  }
  server_.sendContent("]}");
  server_.sendContent("");
}

void WebService::handleOtaUpload() {
  HTTPUpload& upload = server_.upload();
  if (upload.status == UPLOAD_FILE_START) {
    otaUploadAuthorized_ = wifi_->connected();
    otaUploadSuccess_ = otaUploadAuthorized_ && ota_->beginWebUpdate();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (otaUploadSuccess_) otaUploadSuccess_ = ota_->writeWebUpdate(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (otaUploadSuccess_) otaUploadSuccess_ = ota_->endWebUpdate();
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    ota_->abortWebUpdate();
    otaUploadSuccess_ = false;
  }
  yield();
}

void WebService::finishOtaUpload() {
  if (!otaUploadAuthorized_) {
    if (!wifi_->connected()) {
      server_.send(409, "text/plain", "OTA requires a connected STA network\n");
    } else server_.send(403, "text/plain", "OTA upload unavailable\n");
    return;
  }
  if (!otaUploadSuccess_) {
    server_.send(500, "text/plain", "Firmware update failed\n");
    return;
  }
  server_.send(200, "text/plain", "Firmware updated; restarting\n");
  ota_->scheduleRestart();
}

