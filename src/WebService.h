#pragma once

#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WebServer.h>
using NativeWebServer = ESP8266WebServer;
#elif defined(ESP32)
#include <WebServer.h>
using NativeWebServer = WebServer;
#else
#error "No WebService backend for this platform"
#endif

#include "DeviceIdentity.h"
#include "ConfigStore.h"
#include "LogService.h"
#include "WiFiService.h"

class WebService {
 public:
  WebService();
  void begin(const char* projectName, const char* firmwareVersion,
             const DeviceIdentity& identity, ConfigStore& config,
             WiFiService& wifi, LogService& logs);
  void tick();
  NativeWebServer& nativeServer() { return server_; }
  uint32_t heapBeforeBegin() const { return heapBeforeBegin_; }
  uint32_t heapAfterBegin() const { return heapAfterBegin_; }
  uint32_t minimumHeap() const { return minimumHeap_; }

 private:
  static void jsonEscape(const char* input, char* output, size_t capacity);
  static void htmlEscape(const char* input, char* output, size_t capacity);
  void registerRoutes();
  void sendHome();
  void sendStatus();
  void sendLogs();
  void saveWiFi();
  void sendHtmlValue(const char* label, const char* value);
  void sendHtmlNumber(const char* label, long value, const char* unit = "");

  NativeWebServer server_;
  const char* projectName_ = nullptr;
  const char* firmwareVersion_ = nullptr;
  const DeviceIdentity* identity_ = nullptr;
  ConfigStore* config_ = nullptr;
  WiFiService* wifi_ = nullptr;
  LogService* logs_ = nullptr;
  uint32_t heapBeforeBegin_ = 0;
  uint32_t heapAfterBegin_ = 0;
  uint32_t minimumHeap_ = UINT32_MAX;
};

