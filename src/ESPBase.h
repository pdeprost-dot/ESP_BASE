#pragma once

#include <Arduino.h>

#include "ConfigStore.h"
#include "DeviceIdentity.h"
#include "LogService.h"
#include "MqttService.h"
#include "OtaService.h"
#include "Version.h"
#include "WebService.h"
#include "WiFiService.h"

class ESPBase {
 public:
  static constexpr size_t kFirmwareVersionCapacity = 64;

  explicit ESPBase(const char* projectName = ESPBaseDefaults::kFirmwareName,
                   const char* firmwareVersion = ESPBaseDefaults::kFirmwareVersion);

  void begin();
  void loop();
  bool addGetRoute(const char* path, WebRouteHandler handler, void* context = nullptr);
  bool addGetRoute(const char* path, WebRequestRouteHandler handler, void* context = nullptr);
  bool addPostRoute(const char* path, WebRequestRouteHandler handler,
                    void* context = nullptr);
  bool addPage(const char* label, const char* path, WebRouteHandler handler,
               void* context = nullptr);
  bool addPage(const char* label, const char* path, WebRequestRouteHandler handler,
               void* context = nullptr);

  const char* projectName() const { return projectName_; }
  const char* firmwareVersion() const { return firmwareVersion_; }
  const char* deviceId() const { return deviceIdentity_.id(); }
  const char* hostname() const { return deviceIdentity_.hostname(); }
  MqttService& mqtt() { return mqttService_; }
  const MqttService& mqtt() const { return mqttService_; }

 private:
  static void handleOtaStarted(void* context);
  void printHelp();
  void printStatus();
  void handleCommand(char* line);
  void tickSerial();

  char projectName_[33]{};
  char firmwareVersion_[kFirmwareVersionCapacity]{};
  DeviceIdentity deviceIdentity_;
  LogService logService_;
  ConfigStore configStore_;
  WiFiService wifiService_;
  OtaService otaService_;
  MqttService mqttService_;
  WebService webService_;
  char commandBuffer_[128]{};
  size_t commandLength_ = 0;
};

