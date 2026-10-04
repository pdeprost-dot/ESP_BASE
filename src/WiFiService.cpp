#include "WiFiService.h"

#include "PlatformCompat.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>

namespace {
WiFiEventHandler stationConnectedHandler;
WiFiEventHandler stationDisconnectedHandler;
WiFiEventHandler stationGotIpHandler;
WiFiEventHandler stationDhcpTimeoutHandler;

const char* disconnectReasonName(WiFiDisconnectReason reason) {
  switch (reason) {
    case WIFI_DISCONNECT_REASON_NO_AP_FOUND: return "NO_AP_FOUND";
    case WIFI_DISCONNECT_REASON_AUTH_FAIL: return "AUTH_FAIL";
    case WIFI_DISCONNECT_REASON_ASSOC_FAIL: return "ASSOC_FAIL";
    case WIFI_DISCONNECT_REASON_4WAY_HANDSHAKE_TIMEOUT: return "4WAY_HANDSHAKE_TIMEOUT";
    case WIFI_DISCONNECT_REASON_HANDSHAKE_TIMEOUT: return "HANDSHAKE_TIMEOUT";
    case WIFI_DISCONNECT_REASON_BEACON_TIMEOUT: return "BEACON_TIMEOUT";
    case WIFI_DISCONNECT_REASON_AUTH_EXPIRE: return "AUTH_EXPIRE";
    case WIFI_DISCONNECT_REASON_ASSOC_EXPIRE: return "ASSOC_EXPIRE";
    case WIFI_DISCONNECT_REASON_ASSOC_LEAVE: return "ASSOC_LEAVE";
    default: return "OTHER";
  }
}
}  // namespace
#elif defined(ESP32)
#include <WiFi.h>
#else
#error "No WiFiService backend for this platform"
#endif

void WiFiService::begin(const char* hostname, ConfigStore& config, LogService& logs) {
  config_ = &config;
  logs_ = &logs;
  strlcpy(hostname_, hostname, sizeof(hostname_));
  snprintf(apSsid_, sizeof(apSsid_), "%.25s-setup", hostname_);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
#if defined(ESP8266)
  stationConnectedHandler = WiFi.onStationModeConnected(
      [this](const WiFiEventStationModeConnected& event) {
        logs_->add("WIFI", "Event STA associated SSID=%s channel=%u slot=%d",
                   event.ssid.c_str(), static_cast<unsigned>(event.channel),
                   static_cast<int>(attemptedSlot_ + 1));
      });
  stationDisconnectedHandler = WiFi.onStationModeDisconnected(
      [this](const WiFiEventStationModeDisconnected& event) {
        if (state_ == State::DISCONNECTING) disconnectObserved_ = true;
        logs_->add("WIFI", "Event STA disconnected SSID=%s reason=%u (%s) slot=%d",
                   event.ssid.c_str(), static_cast<unsigned>(event.reason),
                   disconnectReasonName(event.reason),
                   static_cast<int>((state_ == State::DISCONNECTING ? disconnectFromSlot_
                                                                    : attemptedSlot_) + 1));
      });
  stationGotIpHandler = WiFi.onStationModeGotIP(
      [this](const WiFiEventStationModeGotIP& event) {
        logs_->add("WIFI", "Event STA DHCP IP=%s gateway=%s slot=%d",
                   event.ip.toString().c_str(), event.gw.toString().c_str(),
                   static_cast<int>(attemptedSlot_ + 1));
      });
  stationDhcpTimeoutHandler = WiFi.onStationModeDHCPTimeout([this]() {
    logs_->add("WIFI", "Event STA DHCP timeout slot=%d",
               static_cast<int>(attemptedSlot_ + 1));
  });
#endif
  if (config_->apAlwaysOn()) startFallbackAp();
  startSelection();
}

void WiFiService::setState(State state) { state_ = state; }

void WiFiService::startSelection() {
  connectedSlot_ = -1;
  if (config_->configured(0)) startStationAttempt(0);
  else if (config_->configured(1)) startStationAttempt(1);
  else startFallbackAp();
}

void WiFiService::startDisconnecting(uint8_t nextSlot) {
  pendingSlot_ = static_cast<int8_t>(nextSlot);
  disconnectFromSlot_ = connectedSlot_ >= 0 ? connectedSlot_ : attemptedSlot_;
  disconnectObserved_ = false;
  disconnectStartedAt_ = millis();
  setState(State::DISCONNECTING);
  logs_->add("WIFI", "Disconnecting STA%u before STA%u",
             static_cast<unsigned>(disconnectFromSlot_ + 1),
             static_cast<unsigned>(nextSlot + 1));
  WiFi.disconnect(false);
}

void WiFiService::startStationAttempt(uint8_t slot) {
  if (!config_->configured(slot)) {
    handleAttemptFailure();
    return;
  }
  WiFi.mode(apActive_ ? WIFI_AP_STA : WIFI_STA);
  PlatformCompat::applyHostname(hostname_);
  const char* targetSsid = config_->ssid(slot);
  const char* targetPassword = config_->password(slot);
  WiFi.begin(targetSsid, targetPassword);
  attemptedSlot_ = static_cast<int8_t>(slot);
  attemptStartedAt_ = millis();
  setState(State::CONNECTING);
  logs_->add("WIFI", "Connecting STA%u SSID=%s", static_cast<unsigned>(slot + 1),
             targetSsid);
}

void WiFiService::handleAttemptFailure() {
  if (attemptedSlot_ != 1 && config_->configured(1)) {
    startDisconnecting(1);
    return;
  }
  attemptedSlot_ = -1;
  WiFi.disconnect(false);
  startFallbackAp();
}

void WiFiService::startFallbackAp() {
  if (!apActive_) {
    WiFi.mode(WIFI_AP_STA);
    apActive_ = WiFi.softAP(apSsid_, config_->apPassword());
    logs_->add("WIFI", "Fallback AP %s SSID=%s", apActive_ ? "ready" : "failed", apSsid_);
    if (apActive_) Serial.println(F("Provisioning AP ready; password is configurable in /wifi"));
  }
  if (!connected()) setState(State::FALLBACK_AP);
  nextAttemptAt_ = millis() + kRetryIntervalMs;
}

void WiFiService::stopFallbackAp() {
  if (!apActive_) return;
  if (!WiFi.enableAP(false)) {
    logs_->add("WIFI", "Fallback AP stop failed");
    return;
  }
  apActive_ = false;
  PlatformCompat::applyHostname(hostname_);
  logs_->add("WIFI", "Fallback AP stopped");
}

void WiFiService::applyApPolicy() {
  if (config_->apAlwaysOn()) startFallbackAp();
  else if (connected()) stopFallbackAp();
}

void WiFiService::tick() {
  if (scanState_ == ScanState::RUNNING) {
    const int result = WiFi.scanComplete();
    if (result >= 0) collectScanResults(result);
#if defined(ESP8266)
    else if (result == WIFI_SCAN_FAILED) scanState_ = ScanState::FAILED;
#else
    else if (result == WIFI_SCAN_FAILED) scanState_ = ScanState::FAILED;
#endif
  }

  const uint32_t now = millis();
  if (state_ == State::DISCONNECTING) {
    const uint32_t elapsed = now - disconnectStartedAt_;
    const bool settledAfterEvent = disconnectObserved_ && elapsed >= kDisconnectSettleMs;
    if (settledAfterEvent || elapsed >= kDisconnectTimeoutMs) {
      const int8_t nextSlot = pendingSlot_;
      logs_->add("WIFI", "Disconnect settling complete after %lu ms event=%s; next=STA%u",
                 static_cast<unsigned long>(elapsed), disconnectObserved_ ? "yes" : "no",
                 static_cast<unsigned>(nextSlot + 1));
      pendingSlot_ = -1;
      startStationAttempt(static_cast<uint8_t>(nextSlot));
    }
    return;
  }
  if (WiFi.status() == WL_CONNECTED) {
    if (state_ != State::CONNECTED) {
      connectedSlot_ = attemptedSlot_;
      setState(State::CONNECTED);
      applyApPolicy();
      logs_->add("WIFI", "Connected STA%u IP=%s RSSI=%d",
                 static_cast<unsigned>(connectedSlot_ + 1),
                 WiFi.localIP().toString().c_str(), WiFi.RSSI());
    }
    return;
  }

  if (state_ == State::CONNECTED) {
    logs_->add("WIFI", "Connection lost from STA%u",
               static_cast<unsigned>(connectedSlot_ + 1));
    startDisconnecting(config_->configured(0) ? 0 : 1);
    return;
  }
  if (state_ == State::CONNECTING && now - attemptStartedAt_ >= kConnectTimeoutMs) {
    logs_->add("WIFI", "STA%u connection timeout status=%d",
               static_cast<unsigned>(attemptedSlot_ + 1), static_cast<int>(WiFi.status()));
    handleAttemptFailure();
    return;
  }
  if (state_ == State::FALLBACK_AP && static_cast<int32_t>(now - nextAttemptAt_) >= 0) {
    startSelection();
  }
}

void WiFiService::credentialsChanged() {
  if (config_->configured(0)) startDisconnecting(0);
  else if (config_->configured(1)) startDisconnecting(1);
  else {
    WiFi.disconnect(false);
    startFallbackAp();
  }
}

void WiFiService::apSettingsChanged(bool passwordChanged) {
  if (config_->apAlwaysOn()) {
    if (apActive_ && passwordChanged) {
      WiFi.softAPdisconnect(true);
      apActive_ = false;
    }
    if (!apActive_) startFallbackAp();
    return;
  }

  if (connected()) {
    stopFallbackAp();
    return;
  }

  if (apActive_ && passwordChanged) {
    WiFi.softAPdisconnect(true);
    apActive_ = false;
  }
  if (!apActive_) startFallbackAp();
}

bool WiFiService::startScan() {
  if (scanState_ == ScanState::RUNNING) return false;
  WiFi.scanDelete();
  scanCount_ = 0;
  const int result = WiFi.scanNetworks(true, true);
  if (result == WIFI_SCAN_RUNNING) {
    scanState_ = ScanState::RUNNING;
    return true;
  }
  if (result >= 0) {
    collectScanResults(result);
    return true;
  }
  scanState_ = ScanState::FAILED;
  return false;
}

void WiFiService::collectScanResults(int count) {
  scanCount_ = 0;
  for (int index = 0; index < count; ++index) {
    const String foundSsid = WiFi.SSID(index);
    if (foundSsid.length() == 0 || foundSsid.length() > 32) continue;
    size_t target = scanCount_;
    for (size_t existing = 0; existing < scanCount_; ++existing) {
      if (strcmp(scanResults_[existing].ssid, foundSsid.c_str()) == 0) {
        target = existing;
        break;
      }
    }
    if (target == scanCount_) {
      if (scanCount_ >= kMaxScanResults) continue;
      ++scanCount_;
    } else if (scanResults_[target].rssi >= WiFi.RSSI(index)) {
      continue;
    }
    strlcpy(scanResults_[target].ssid, foundSsid.c_str(), sizeof(scanResults_[target].ssid));
    scanResults_[target].rssi = WiFi.RSSI(index);
#if defined(ESP8266)
    scanResults_[target].encrypted = WiFi.encryptionType(index) != ENC_TYPE_NONE;
#else
    scanResults_[target].encrypted = WiFi.encryptionType(index) != WIFI_AUTH_OPEN;
#endif
  }
  WiFi.scanDelete();
  scanState_ = ScanState::READY;
  logs_->add("WIFI", "Scan complete networks=%u", static_cast<unsigned>(scanCount_));
}

void WiFiService::clearScan() {
  WiFi.scanDelete();
  scanCount_ = 0;
  scanState_ = ScanState::IDLE;
}

const char* WiFiService::stateName() const {
  switch (state_) {
    case State::IDLE: return "IDLE";
    case State::DISCONNECTING: return "DISCONNECTING";
    case State::CONNECTING: return attemptedSlot_ == 1 ? "CONNECTING_STA2" : "CONNECTING_STA1";
    case State::CONNECTED: return connectedSlot_ == 1 ? "CONNECTED_STA2" : "CONNECTED_STA1";
    case State::FALLBACK_AP: return "FALLBACK_AP";
  }
  return "UNKNOWN";
}

const char* WiFiService::scanStateName() const {
  switch (scanState_) {
    case ScanState::IDLE: return "IDLE";
    case ScanState::RUNNING: return "RUNNING";
    case ScanState::READY: return "READY";
    case ScanState::FAILED: return "FAILED";
  }
  return "UNKNOWN";
}

bool WiFiService::connected() const { return WiFi.status() == WL_CONNECTED; }
String WiFiService::stationIp() const { return connected() ? WiFi.localIP().toString() : String(); }
String WiFiService::apIp() const { return apActive_ ? WiFi.softAPIP().toString() : String(); }
String WiFiService::currentSsid() const { return connected() ? WiFi.SSID() : String(); }
int32_t WiFiService::rssi() const { return connected() ? WiFi.RSSI() : 0; }
const char* WiFiService::modeName() const {
  if (apActive_ && connected()) return "AP+STA";
  if (apActive_) return "AP";
  return "STA";
}
