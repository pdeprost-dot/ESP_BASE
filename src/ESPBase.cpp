#include "ESPBase.h"

#include "PlatformCompat.h"

ESPBase::ESPBase(const char* projectName, const char* firmwareVersion) {
  strlcpy(projectName_, projectName ? projectName : ESPBaseDefaults::kFirmwareName,
          sizeof(projectName_));
  strlcpy(firmwareVersion_, firmwareVersion ? firmwareVersion : ESPBaseDefaults::kFirmwareVersion,
          sizeof(firmwareVersion_));
}

bool ESPBase::addGetRoute(const char* path, WebRouteHandler handler, void* context) {
  return webService_.addGetRoute(path, handler, context);
}

bool ESPBase::addPage(const char* label, const char* path, WebRouteHandler handler,
                      void* context) {
  return webService_.addPage(label, path, handler, context);
}

void ESPBase::begin() {
  Serial.begin(115200);
  Serial.println();
  deviceIdentity_.begin();
  logService_.begin();
  configStore_.begin();
  logService_.add("SYSTEM", "%s %s boot id=%s hostname=%s", projectName_, firmwareVersion_,
                  deviceIdentity_.id(), deviceIdentity_.hostname());
  logService_.add("SYSTEM", "free_heap=%lu",
                  static_cast<unsigned long>(PlatformCompat::freeHeap()));
  wifiService_.begin(deviceIdentity_.hostname(), configStore_, logService_);
  otaService_.begin(deviceIdentity_.hostname(), nullptr, logService_);
  webService_.begin(projectName_, firmwareVersion_, deviceIdentity_, configStore_,
                    wifiService_, otaService_, logService_);
  printHelp();
}

void ESPBase::loop() {
  tickSerial();
  if (!otaService_.busy()) wifiService_.tick();
  otaService_.tick(wifiService_.connected());
  webService_.tick();
  yield();
}

void ESPBase::printHelp() {
  Serial.println(F("Commands: WIFI <ssid>|<password>, STATUS, LOGS, CLEAR, HELP"));
}

void ESPBase::printStatus() {
  Serial.print(F("device_id=")); Serial.println(deviceIdentity_.id());
  Serial.print(F("hostname=")); Serial.println(deviceIdentity_.hostname());
  Serial.print(F("free_heap=")); Serial.println(PlatformCompat::freeHeap());
  Serial.print(F("wifi_state=")); Serial.println(wifiService_.stateName());
  Serial.print(F("sta_configured="));
  Serial.println(configStore_.configured() ? F("yes") : F("no"));
  Serial.print(F("sta2_configured="));
  Serial.println(configStore_.configured(1) ? F("yes") : F("no"));
  Serial.print(F("connected_slot=")); Serial.println(wifiService_.connectedSlot() + 1);
  Serial.print(F("sta_ip=")); Serial.println(wifiService_.stationIp());
  Serial.print(F("ap_active=")); Serial.println(wifiService_.apActive() ? F("yes") : F("no"));
  if (wifiService_.apActive()) {
    Serial.print(F("ap_ssid=")); Serial.println(wifiService_.apSsid());
    Serial.print(F("ap_ip=")); Serial.println(wifiService_.apIp());
  }
}

void ESPBase::handleCommand(char* line) {
  if (strcmp(line, "HELP") == 0) {
    printHelp();
  } else if (strcmp(line, "STATUS") == 0) {
    printStatus();
  } else if (strcmp(line, "LOGS") == 0) {
    logService_.printTo(Serial);
  } else if (strcmp(line, "CLEAR") == 0) {
    if (configStore_.clear()) {
      logService_.add("CONFIG", "Wi-Fi configuration cleared");
      wifiService_.credentialsChanged();
    } else {
      logService_.add("CONFIG", "Failed to clear configuration");
    }
  } else if (strncmp(line, "WIFI ", 5) == 0) {
    char* separator = strchr(line + 5, '|');
    if (!separator) {
      Serial.println(F("ERROR expected WIFI <ssid>|<password>"));
      return;
    }
    *separator = '\0';
    const char* ssid = line + 5;
    const char* password = separator + 1;
    if (!configStore_.saveWiFi(ssid, password)) {
      Serial.println(F("ERROR invalid or unsaved Wi-Fi configuration"));
      return;
    }
    logService_.add("CONFIG", "Wi-Fi configuration saved for SSID %s", ssid);
    wifiService_.credentialsChanged();
  } else if (line[0] != '\0') {
    Serial.println(F("ERROR unknown command"));
    printHelp();
  }
}

void ESPBase::tickSerial() {
  while (Serial.available()) {
    const char value = static_cast<char>(Serial.read());
    if (value == '\r') continue;
    if (value == '\n') {
      commandBuffer_[commandLength_] = '\0';
      handleCommand(commandBuffer_);
      commandLength_ = 0;
    } else if (commandLength_ + 1 < sizeof(commandBuffer_)) {
      commandBuffer_[commandLength_++] = value;
    } else {
      commandLength_ = 0;
      Serial.println(F("ERROR command too long"));
    }
  }
}

