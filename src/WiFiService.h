#pragma once

#include <Arduino.h>

#include "ConfigStore.h"
#include "LogService.h"

class WiFiService {
 public:
  enum class State : uint8_t { IDLE, DISCONNECTING, CONNECTING, CONNECTED, FALLBACK_AP };
  enum class ScanState : uint8_t { IDLE, RUNNING, READY, FAILED };
  struct ScanResult {
    char ssid[33]{};
    int32_t rssi = 0;
    bool encrypted = true;
  };

  void begin(const char* hostname, ConfigStore& config, LogService& logs);
  void tick();
  void credentialsChanged();
  void apSettingsChanged(bool passwordChanged);
  bool startScan();
  void clearScan();
  State state() const { return state_; }
  const char* stateName() const;
  bool connected() const;
  int8_t connectedSlot() const { return connectedSlot_; }
  int8_t attemptedSlot() const { return attemptedSlot_; }
  bool apActive() const { return apActive_; }
  const char* apSsid() const { return apSsid_; }
  String stationIp() const;
  String apIp() const;
  String currentSsid() const;
  int32_t rssi() const;
  const char* modeName() const;
  ScanState scanState() const { return scanState_; }
  const char* scanStateName() const;
  size_t scanCount() const { return scanCount_; }
  const ScanResult& scanResult(size_t index) const { return scanResults_[index]; }

 private:
  static constexpr uint32_t kConnectTimeoutMs = 15000;
  static constexpr uint32_t kDisconnectSettleMs = 250;
  static constexpr uint32_t kDisconnectTimeoutMs = 1000;
  static constexpr uint32_t kRetryIntervalMs = 60000;
  static constexpr size_t kMaxScanResults = 12;

  void startSelection();
  void startDisconnecting(uint8_t nextSlot);
  void startStationAttempt(uint8_t slot);
  void handleAttemptFailure();
  void startFallbackAp();
  void stopFallbackAp();
  void applyApPolicy();
  void collectScanResults(int count);
  void setState(State state);

  ConfigStore* config_ = nullptr;
  LogService* logs_ = nullptr;
  State state_ = State::IDLE;
  ScanState scanState_ = ScanState::IDLE;
  uint32_t attemptStartedAt_ = 0;
  uint32_t disconnectStartedAt_ = 0;
  uint32_t nextAttemptAt_ = 0;
  int8_t attemptedSlot_ = -1;
  int8_t connectedSlot_ = -1;
  int8_t pendingSlot_ = -1;
  int8_t disconnectFromSlot_ = -1;
  bool disconnectObserved_ = false;
  bool apActive_ = false;
  char hostname_[32]{};
  char apSsid_[33]{};
  ScanResult scanResults_[kMaxScanResults];
  size_t scanCount_ = 0;
};
