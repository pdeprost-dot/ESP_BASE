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
#include "OtaService.h"
#include "MqttService.h"
#include "WebResponse.h"
#include "WiFiService.h"

class WebService {
 public:
  WebService();
  bool addGetRoute(const char* path, WebRouteHandler handler, void* context);
  bool addGetRoute(const char* path, WebRequestRouteHandler handler, void* context);
  bool addPostRoute(const char* path, WebRequestRouteHandler handler, void* context);
  bool addPage(const char* label, const char* path, WebRouteHandler handler, void* context);
  bool addPage(const char* label, const char* path, WebRequestRouteHandler handler, void* context);
  void begin(const char* projectName, const char* firmwareVersion,
             const DeviceIdentity& identity, ConfigStore& config,
             WiFiService& wifi, OtaService& ota, MqttService& mqtt, LogService& logs);
  void tick();
  NativeWebServer& nativeServer() { return server_; }
  uint32_t heapBeforeBegin() const { return heapBeforeBegin_; }
  uint32_t heapAfterBegin() const { return heapAfterBegin_; }
  uint32_t minimumHeap() const { return minimumHeap_; }

 private:
  static constexpr size_t kMaxApplicationRoutes = 4;
  static constexpr size_t kMaxRoutePathLength = 47;
  static constexpr size_t kMaxPageLabelLength = 20;

  struct ApplicationRoute {
    union Handler {
      Handler() : legacy(nullptr) {}
      WebRouteHandler legacy;
      WebRequestRouteHandler request;
    } handler;
    char path[kMaxRoutePathLength + 1]{};
    char label[kMaxPageLabelLength + 1]{};
    void* context = nullptr;
    bool requestAware = false;
    bool postRoute = false;
  };

  static void jsonEscape(const char* input, char* output, size_t capacity);
  static void htmlEscape(const char* input, char* output, size_t capacity);
  static void sendApplicationResponse(void* context, uint16_t statusCode,
                                      const char* contentType, const char* body);
  static void beginApplicationPage(void* context, const char* title);
  static void writeApplicationPage(void* context, const char* html);
  static void endApplicationPage(void* context);
  static bool applicationRequestHasArg(void* context, const char* name);
  static String applicationRequestArg(void* context, const char* name);
  static bool isReservedRoute(const char* path);
  bool addApplicationRoute(const char* label, const char* path,
                           WebRouteHandler legacyHandler,
                           WebRequestRouteHandler requestHandler, void* context,
                           bool postRoute);
  void registerRoutes();
  void dispatchApplicationRoute(size_t index);
  void beginPage(const char* title, const char* activePath);
  void endPage();
  void sendNavItem(const char* label, const char* path, const char* activePath);
  void sendHome();
  void sendSetupPage();
  void sendWifiPage();
  void sendLogsPage();
  void sendSystemPage();
  void sendOtaPage();
  void sendMqttPage();
  void sendStatus();
  void sendLogs();
  void saveWiFi();
  void saveWifiSetup();
  void saveWifiConfig();
  void saveApPassword();
  void saveMqtt();
  void startWifiScan();
  void sendWifiScan();
  void finishOtaUpload();
  void handleOtaUpload();
  void sendWifiForm();
  void sendScanPanel();
  void sendHtmlValue(const char* label, const char* value);
  void sendHtmlNumber(const char* label, long value, const char* unit = "");

  NativeWebServer server_;
  const char* projectName_ = nullptr;
  const char* firmwareVersion_ = nullptr;
  const DeviceIdentity* identity_ = nullptr;
  ConfigStore* config_ = nullptr;
  WiFiService* wifi_ = nullptr;
  OtaService* ota_ = nullptr;
  MqttService* mqtt_ = nullptr;
  LogService* logs_ = nullptr;
  uint32_t heapBeforeBegin_ = 0;
  uint32_t heapAfterBegin_ = 0;
  uint32_t minimumHeap_ = UINT32_MAX;
  ApplicationRoute applicationRoutes_[kMaxApplicationRoutes];
  size_t applicationRouteCount_ = 0;
  bool started_ = false;
  const char* activeApplicationPath_ = nullptr;
  bool otaUploadAuthorized_ = false;
  bool otaUploadSuccess_ = false;
  bool wifiChangePending_ = false;
  uint32_t wifiChangeAt_ = 0;
  bool apPasswordChangePending_ = false;
  bool apPasswordChangedPending_ = false;
  uint32_t apPasswordChangeAt_ = 0;
};

