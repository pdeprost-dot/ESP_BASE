#include "PlatformCompat.h"
#include "NetworkAvailability.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <ETH.h>
#else
#error "ESP_BASE currently supports ESP8266; ESP32 port is planned"
#endif

namespace PlatformCompat {

uint64_t hardwareId() {
#if defined(ESP8266)
  return ESP.getChipId();
#else
  return ESP.getEfuseMac();
#endif
}

uint32_t freeHeap() { return ESP.getFreeHeap(); }

bool networkConnected() {
#if defined(ESP32)
  const IPAddress address = ETH.localIP();
  return ESPBaseNetwork::available(WiFi.status() == WL_CONNECTED, ETH.linkUp(),
                                   address != IPAddress(0, 0, 0, 0));
#else
  return ESPBaseNetwork::available(WiFi.status() == WL_CONNECTED, false, false);
#endif
}

void applyHostname(const char* hostname) {
#if defined(ESP8266)
  WiFi.hostname(hostname);
#else
  WiFi.setHostname(hostname);
#endif
}

const char* platformName() {
#if defined(ESP8266)
  return "esp8266";
#else
  return "esp32";
#endif
}

}  // namespace PlatformCompat

