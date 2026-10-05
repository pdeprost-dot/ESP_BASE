#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPBase.h>
#include <LittleFS.h>
#include <time.h>

namespace {

constexpr char kConfigPath[] = "/ntp.cfg";
constexpr char kTempConfigPath[] = "/ntp.tmp";
constexpr uint32_t kPublishIntervalMs = 30000UL;
constexpr time_t kPlausibleEpoch = 1609459200;

struct NtpConfiguration {
  char server[64];
  char timezone[64];
};

ESPBase espBase("NTP System Monitor", "1.0.0");
NtpConfiguration ntpConfig{"pool.ntp.org", "CET-1CEST,M3.5.0,M10.5.0/3"};
bool fileSystemReady = false;
uint32_t lastPublishAt = 0;

bool copyChecked(char* destination, size_t capacity, const char* source) {
  if (!destination || !source || capacity == 0 || strlen(source) >= capacity) return false;
  strlcpy(destination, source, capacity);
  return true;
}

bool validNtpServer(const char* value) {
  if (!value) return false;
  const size_t length = strlen(value);
  if (length == 0 || length >= sizeof(ntpConfig.server)) return false;
  for (size_t index = 0; index < length; ++index) {
    const char valueAtIndex = value[index];
    if (!(isalnum(static_cast<unsigned char>(valueAtIndex)) || valueAtIndex == '.' ||
          valueAtIndex == '-' || valueAtIndex == ':')) {
      return false;
    }
  }
  return true;
}

bool validTimezone(const char* value) {
  if (!value) return false;
  const size_t length = strlen(value);
  if (length == 0 || length >= sizeof(ntpConfig.timezone)) return false;
  for (size_t index = 0; index < length; ++index) {
    const unsigned char valueAtIndex = static_cast<unsigned char>(value[index]);
    if (valueAtIndex < 0x21 || valueAtIndex > 0x7e ||
        valueAtIndex == '&' || valueAtIndex == '=') {
      return false;
    }
  }
  return true;
}

void htmlEscape(const char* source, char* destination, size_t capacity) {
  if (!destination || capacity == 0) return;
  size_t used = 0;
  destination[0] = '\0';
  for (const char* cursor = source ? source : "";
       *cursor && used + 1 < capacity; ++cursor) {
    const char* replacement = nullptr;
    switch (*cursor) {
      case '&': replacement = "&amp;"; break;
      case '<': replacement = "&lt;"; break;
      case '>': replacement = "&gt;"; break;
      case '"': replacement = "&quot;"; break;
      case '\'': replacement = "&#39;"; break;
      default:
        destination[used++] = *cursor;
        destination[used] = '\0';
        continue;
    }
    const size_t replacementLength = strlen(replacement);
    if (used + replacementLength >= capacity) break;
    memcpy(destination + used, replacement, replacementLength);
    used += replacementLength;
    destination[used] = '\0';
  }
}

void formatLocalTime(char* output, size_t capacity) {
  if (!output || capacity == 0) return;
  const time_t now = time(nullptr);
  if (now < kPlausibleEpoch) {
    strlcpy(output, "En attente de synchronisation NTP", capacity);
    return;
  }
  struct tm local {};
  localtime_r(&now, &local);
  if (strftime(output, capacity, "%Y-%m-%d %H:%M:%S %Z", &local) == 0) {
    strlcpy(output, "Heure indisponible", capacity);
  }
}

void applyNtpConfiguration() {
  configTime(ntpConfig.timezone, ntpConfig.server);
}

bool loadNtpConfiguration() {
  if (!fileSystemReady || !LittleFS.exists(kConfigPath)) return false;
  File file = LittleFS.open(kConfigPath, "r");
  if (!file) return false;

  char content[160]{};
  const size_t length = file.readBytes(content, sizeof(content) - 1);
  file.close();
  content[length] = '\0';

  char loadedServer[sizeof(ntpConfig.server)]{};
  char loadedTimezone[sizeof(ntpConfig.timezone)]{};
  char* savePointer = nullptr;
  for (char* line = strtok_r(content, "\n", &savePointer); line;
       line = strtok_r(nullptr, "\n", &savePointer)) {
    if (strncmp(line, "server=", 7) == 0) {
      copyChecked(loadedServer, sizeof(loadedServer), line + 7);
    } else if (strncmp(line, "timezone=", 9) == 0) {
      copyChecked(loadedTimezone, sizeof(loadedTimezone), line + 9);
    }
  }
  if (!validNtpServer(loadedServer) || !validTimezone(loadedTimezone)) return false;
  copyChecked(ntpConfig.server, sizeof(ntpConfig.server), loadedServer);
  copyChecked(ntpConfig.timezone, sizeof(ntpConfig.timezone), loadedTimezone);
  return true;
}

bool saveNtpConfiguration(const NtpConfiguration& value) {
  if (!fileSystemReady) return false;
  File file = LittleFS.open(kTempConfigPath, "w");
  if (!file) return false;
  const bool written = file.printf("version=1\nserver=%s\ntimezone=%s\n",
                                   value.server, value.timezone) > 0;
  file.close();
  if (!written) {
    LittleFS.remove(kTempConfigPath);
    return false;
  }
  LittleFS.remove(kConfigPath);
  if (!LittleFS.rename(kTempConfigPath, kConfigPath)) {
    LittleFS.remove(kTempConfigPath);
    return false;
  }
  return true;
}

void handleNtpPage(WebResponse& response, void*) {
  char escapedServer[384]{};
  char escapedTimezone[384]{};
  char localTime[64]{};
  htmlEscape(ntpConfig.server, escapedServer, sizeof(escapedServer));
  htmlEscape(ntpConfig.timezone, escapedTimezone, sizeof(escapedTimezone));
  formatLocalTime(localTime, sizeof(localTime));

  response.beginPage("NTP System Monitor");
  response.write("<section class='card'><h2>Heure NTP</h2><p><strong>Heure locale :</strong> ");
  response.write(localTime);
  response.write("</p><p><strong>Serveur :</strong> ");
  response.write(escapedServer);
  response.write("</p><p><strong>Fuseau POSIX :</strong> ");
  response.write(escapedTimezone);
  response.write("</p></section><section class='card'><h2>Configuration</h2>"
                 "<form method='post' action='/ntp-config'><label>Serveur NTP"
                 "<input name='server' maxlength='63' required value='");
  response.write(escapedServer);
  response.write("'></label><label>Fuseau horaire POSIX"
                 "<input name='timezone' maxlength='63' required value='");
  response.write(escapedTimezone);
  response.write("'></label><button type='submit'>Enregistrer</button></form></section>");
  response.endPage();
}

void handleNtpConfigurationPost(WebRequest& request, WebResponse& response, void*) {
  if (!request.hasArg("server") || !request.hasArg("timezone")) {
    response.sendText("Parametres manquants", 400);
    return;
  }
  const String server = request.arg("server");
  const String timezone = request.arg("timezone");
  if (!validNtpServer(server.c_str()) || !validTimezone(timezone.c_str())) {
    response.sendText("Serveur ou fuseau invalide", 400);
    return;
  }

  NtpConfiguration candidate{};
  copyChecked(candidate.server, sizeof(candidate.server), server.c_str());
  copyChecked(candidate.timezone, sizeof(candidate.timezone), timezone.c_str());
  if (!saveNtpConfiguration(candidate)) {
    response.sendText("Impossible d'ecrire la configuration", 500);
    return;
  }
  ntpConfig = candidate;
  applyNtpConfiguration();
  response.sendText("Configuration enregistree");
}

void publishStatusIfDue() {
  const uint32_t nowMs = millis();
  if (static_cast<uint32_t>(nowMs - lastPublishAt) < kPublishIntervalMs) return;
  lastPublishAt = nowMs;
  if (!espBase.mqtt().enabled() || !espBase.mqtt().connected()) return;

  char localTime[64]{};
  char payload[384]{};
  formatLocalTime(localTime, sizeof(localTime));
  const IPAddress ip = WiFi.localIP();
  snprintf(payload, sizeof(payload),
           "{\"device_id\":\"%s\",\"hostname\":\"%s\",\"firmware\":\"%s\","
           "\"local_time\":\"%s\",\"uptime_s\":%lu,\"free_heap\":%lu,"
           "\"wifi_rssi_dbm\":%ld,\"ip\":\"%u.%u.%u.%u\"}",
           espBase.deviceId(), espBase.hostname(), espBase.firmwareVersion(), localTime,
           static_cast<unsigned long>(nowMs / 1000UL),
           static_cast<unsigned long>(ESP.getFreeHeap()),
           static_cast<long>(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0),
           ip[0], ip[1], ip[2], ip[3]);
  espBase.mqtt().publish("ntp/status", payload, true);
}

}  // namespace

void setup() {
  espBase.addPage("NTP", "/ntp", handleNtpPage);
  espBase.addPostRoute("/ntp-config", handleNtpConfigurationPost);
  espBase.begin();

  fileSystemReady = LittleFS.begin();
  if (!fileSystemReady) {
    Serial.println(F("[APP] LittleFS unavailable; defaults are not persistent"));
  } else {
    loadNtpConfiguration();
  }
  applyNtpConfiguration();
}

void loop() {
  espBase.loop();
  publishStatusIfDue();
}
