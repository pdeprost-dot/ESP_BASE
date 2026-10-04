#include "src/ConfigStore.h"
#include "src/DeviceIdentity.h"
#include "src/LogService.h"
#include "src/WiFiService.h"

namespace {
DeviceIdentity deviceIdentity;
LogService logService;
ConfigStore configStore;
WiFiService wifiService;

char commandBuffer[128]{};
size_t commandLength = 0;

void printHelp() {
  Serial.println(F("Commands: WIFI <ssid>|<password>, STATUS, LOGS, CLEAR, HELP"));
}

void printStatus() {
  Serial.print(F("device_id=")); Serial.println(deviceIdentity.id());
  Serial.print(F("hostname=")); Serial.println(deviceIdentity.hostname());
  Serial.print(F("free_heap=")); Serial.println(PlatformCompat::freeHeap());
  Serial.print(F("wifi_state=")); Serial.println(wifiService.stateName());
  Serial.print(F("sta_configured=")); Serial.println(configStore.configured() ? F("yes") : F("no"));
  Serial.print(F("sta_ip=")); Serial.println(wifiService.stationIp());
  Serial.print(F("ap_active=")); Serial.println(wifiService.apActive() ? F("yes") : F("no"));
  if (wifiService.apActive()) {
    Serial.print(F("ap_ssid=")); Serial.println(wifiService.apSsid());
    Serial.print(F("ap_ip=")); Serial.println(wifiService.apIp());
  }
}

void handleCommand(char* line) {
  if (strcmp(line, "HELP") == 0) {
    printHelp();
  } else if (strcmp(line, "STATUS") == 0) {
    printStatus();
  } else if (strcmp(line, "LOGS") == 0) {
    logService.printTo(Serial);
  } else if (strcmp(line, "CLEAR") == 0) {
    if (configStore.clear()) {
      logService.add("CONFIG", "Wi-Fi configuration cleared");
      wifiService.credentialsChanged();
    } else {
      logService.add("CONFIG", "Failed to clear configuration");
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
    if (!configStore.saveWiFi(ssid, password)) {
      Serial.println(F("ERROR invalid or unsaved Wi-Fi configuration"));
      return;
    }
    logService.add("CONFIG", "Wi-Fi configuration saved for SSID %s", ssid);
    wifiService.credentialsChanged();
  } else if (line[0] != '\0') {
    Serial.println(F("ERROR unknown command"));
    printHelp();
  }
}

void tickSerial() {
  while (Serial.available()) {
    const char value = static_cast<char>(Serial.read());
    if (value == '\r') continue;
    if (value == '\n') {
      commandBuffer[commandLength] = '\0';
      handleCommand(commandBuffer);
      commandLength = 0;
    } else if (commandLength + 1 < sizeof(commandBuffer)) {
      commandBuffer[commandLength++] = value;
    } else {
      commandLength = 0;
      Serial.println(F("ERROR command too long"));
    }
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.println();
  deviceIdentity.begin();
  logService.begin();
  configStore.begin();
  logService.add("SYSTEM", "ESP_BASE boot id=%s hostname=%s",
                 deviceIdentity.id(), deviceIdentity.hostname());
  logService.add("SYSTEM", "free_heap=%lu",
                 static_cast<unsigned long>(PlatformCompat::freeHeap()));
  wifiService.begin(deviceIdentity.hostname(), configStore, logService);
  printHelp();
}

void loop() {
  tickSerial();
  wifiService.tick();
  yield();
}

