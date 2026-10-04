#pragma once

#include <Arduino.h>

#include "ConfigStore.h"
#include "LogService.h"

class WiFiService {
 public:
  enum class State : uint8_t { IDLE, CONNECTING, CONNECTED, FALLBACK_AP };

  void begin(const char* hostname, ConfigStore& config, LogService& logs);
  void tick();
  void credentialsChanged();
  State state() const { return state_; }
  const char* stateName() const;
  bool apActive() const { return apActive_; }
  const char* apSsid() const { return apSsid_; }
  String stationIp() const;
  String apIp() const;

 private:
  static constexpr uint32_t kConnectTimeoutMs = 15000;
  static constexpr uint32_t kRetryIntervalMs = 60000;

  void startStationAttempt();
  void startFallbackAp();
  void stopFallbackAp();
  void setState(State state);

  ConfigStore* config_ = nullptr;
  LogService* logs_ = nullptr;
  State state_ = State::IDLE;
  uint32_t attemptStartedAt_ = 0;
  uint32_t nextAttemptAt_ = 0;
  bool apActive_ = false;
  char hostname_[32]{};
  char apSsid_[33]{};
  char apPassword_[17]{};
};

