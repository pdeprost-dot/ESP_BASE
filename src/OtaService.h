#pragma once

#include <Arduino.h>

#include "LogService.h"

using OtaStartHandler = void (*)(void* context);

class OtaService {
 public:
  void begin(const char* hostname, const char* password, LogService& logs);
  void setStartHandler(OtaStartHandler handler, void* context = nullptr);
  void tick(bool stationConnected);

  bool beginWebUpdate();
  bool writeWebUpdate(uint8_t* data, size_t length);
  bool endWebUpdate();
  void abortWebUpdate();
  void scheduleRestart();

  bool enabled() const { return enabled_; }
  bool busy() const { return arduinoOtaActive_ || webUpdateActive_ || restartPending_; }
  bool webUpdateActive() const { return webUpdateActive_; }
  size_t bytesWritten() const { return bytesWritten_; }
  uint8_t errorCode() const { return errorCode_; }
  uint32_t minimumHeap() const { return minimumHeap_; }

 private:
  void notifyStarted();

  LogService* logs_ = nullptr;
  OtaStartHandler startHandler_ = nullptr;
  void* startContext_ = nullptr;
  bool enabled_ = false;
  bool arduinoOtaStarted_ = false;
  bool arduinoOtaActive_ = false;
  bool webUpdateActive_ = false;
  bool restartPending_ = false;
  uint32_t restartAt_ = 0;
  size_t bytesWritten_ = 0;
  uint8_t errorCode_ = 0;
  uint8_t lastArduinoProgress_ = 0;
  uint32_t minimumHeap_ = UINT32_MAX;
  char hostname_[33]{};
};
