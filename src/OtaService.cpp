#include "OtaService.h"

#include "PlatformCompat.h"

#include <ArduinoOTA.h>
#if defined(ESP8266)
#include <Updater.h>
#else
#include <Update.h>
#endif

void OtaService::begin(const char* hostname, const char* password, LogService& logs) {
  logs_ = &logs;
  strlcpy(hostname_, hostname ? hostname : "esp-base", sizeof(hostname_));
  (void)password;
  enabled_ = true;
  minimumHeap_ = PlatformCompat::freeHeap();
}

void OtaService::tick(bool stationConnected) {
  const uint32_t heap = PlatformCompat::freeHeap();
  if (heap < minimumHeap_) minimumHeap_ = heap;

  if (restartPending_ && static_cast<int32_t>(millis() - restartAt_) >= 0) {
    ESP.restart();
    return;
  }
  if (!enabled_ || !stationConnected || webUpdateActive_) return;
  if (!arduinoOtaStarted_) {
    ArduinoOTA.setHostname(hostname_);
    ArduinoOTA.onStart([this]() {
      arduinoOtaActive_ = true;
      lastArduinoProgress_ = 0;
      logs_->add("OTA", "ArduinoOTA started");
    });
    ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
      if (total == 0) return;
      const uint8_t percent = static_cast<uint8_t>((progress * 100U) / total);
      const uint8_t milestone = static_cast<uint8_t>((percent / 10U) * 10U);
      if (milestone >= 10U && milestone > lastArduinoProgress_) {
        lastArduinoProgress_ = milestone;
        logs_->add("OTA", "ArduinoOTA progress=%u%%", static_cast<unsigned>(milestone));
      }
    });
    ArduinoOTA.onEnd([this]() {
      logs_->add("OTA", "ArduinoOTA completed");
    });
    ArduinoOTA.onError([this](ota_error_t error) {
      arduinoOtaActive_ = false;
      logs_->add("OTA", "ArduinoOTA error=%u", static_cast<unsigned int>(error));
    });
    ArduinoOTA.begin();
    arduinoOtaStarted_ = true;
    logs_->add("OTA", "ArduinoOTA ready");
  }
  ArduinoOTA.handle();
}

bool OtaService::beginWebUpdate() {
  if (!enabled_ || busy()) return false;
  bytesWritten_ = 0;
  errorCode_ = 0;
#if defined(ESP8266)
  const size_t capacity = (ESP.getFreeSketchSpace() - 0x1000U) & 0xFFFFF000U;
  const bool started = Update.begin(capacity);
#else
  const bool started = Update.begin(UPDATE_SIZE_UNKNOWN);
#endif
  if (!started) {
    errorCode_ = Update.getError();
    logs_->add("OTA", "Web update start failed error=%u", errorCode_);
    return false;
  }
  webUpdateActive_ = true;
  logs_->add("OTA", "Web update started");
  return true;
}

bool OtaService::writeWebUpdate(uint8_t* data, size_t length) {
  if (!webUpdateActive_ || !data || length == 0) return false;
  const size_t written = Update.write(data, length);
  bytesWritten_ += written;
  if (written == length) return true;
  errorCode_ = Update.getError();
  logs_->add("OTA", "Web update write failed error=%u", errorCode_);
  Update.end(false);
  webUpdateActive_ = false;
  return false;
}

bool OtaService::endWebUpdate() {
  if (!webUpdateActive_) return false;
  const bool success = Update.end(true);
  webUpdateActive_ = false;
  if (!success) {
    errorCode_ = Update.getError();
    logs_->add("OTA", "Web update finalize failed error=%u", errorCode_);
    return false;
  }
  logs_->add("OTA", "Web update completed bytes=%lu",
             static_cast<unsigned long>(bytesWritten_));
  return true;
}

void OtaService::abortWebUpdate() {
  if (webUpdateActive_) {
#if defined(ESP8266)
    Update.end(false);
#else
    Update.abort();
#endif
  }
  webUpdateActive_ = false;
  errorCode_ = Update.getError();
  logs_->add("OTA", "Web update aborted error=%u", errorCode_);
}

void OtaService::scheduleRestart() {
  restartPending_ = true;
  restartAt_ = millis() + 1200U;
}
