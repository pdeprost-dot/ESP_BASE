#include "WiFiService.h"

#include "PlatformCompat.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
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
  const size_t hostnameLength = strlen(hostname_);
  const char* suffix = hostnameLength >= 6 ? hostname_ + hostnameLength - 6 : hostname_;
  snprintf(apPassword_, sizeof(apPassword_), "ESPbase-%.6s", suffix);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  if (config_->configured()) startStationAttempt();
  else startFallbackAp();
}

void WiFiService::setState(State state) { state_ = state; }

void WiFiService::startStationAttempt() {
  if (!config_->configured()) {
    startFallbackAp();
    return;
  }
  WiFi.mode(apActive_ ? WIFI_AP_STA : WIFI_STA);
  PlatformCompat::applyHostname(hostname_);
  WiFi.begin(config_->ssid(), config_->password());
  attemptStartedAt_ = millis();
  setState(State::CONNECTING);
  logs_->add("WIFI", "Connecting to SSID %s", config_->ssid());
}

void WiFiService::startFallbackAp() {
  if (!apActive_) {
    WiFi.mode(WIFI_AP_STA);
    apActive_ = WiFi.softAP(apSsid_, apPassword_);
    logs_->add("WIFI", "Fallback AP %s SSID=%s", apActive_ ? "ready" : "failed", apSsid_);
    if (apActive_) {
      Serial.print(F("Provisioning AP password: "));
      Serial.println(apPassword_);
    }
  }
  setState(State::FALLBACK_AP);
  nextAttemptAt_ = millis() + kRetryIntervalMs;
}

void WiFiService::stopFallbackAp() {
  if (!apActive_) return;
  WiFi.softAPdisconnect(true);
  apActive_ = false;
  WiFi.mode(WIFI_STA);
  PlatformCompat::applyHostname(hostname_);
  logs_->add("WIFI", "Fallback AP stopped");
}

void WiFiService::tick() {
  const uint32_t now = millis();
  if (WiFi.status() == WL_CONNECTED) {
    if (state_ != State::CONNECTED) {
      stopFallbackAp();
      setState(State::CONNECTED);
      logs_->add("WIFI", "Connected IP=%s RSSI=%d",
                 WiFi.localIP().toString().c_str(), WiFi.RSSI());
    }
    return;
  }

  if (!config_->configured()) {
    if (!apActive_) startFallbackAp();
    return;
  }

  if (state_ == State::CONNECTED) {
    logs_->add("WIFI", "Connection lost");
    startStationAttempt();
    return;
  }

  if (state_ == State::CONNECTING && now - attemptStartedAt_ >= kConnectTimeoutMs) {
    logs_->add("WIFI", "Connection timeout");
    startFallbackAp();
    return;
  }

  if (state_ == State::FALLBACK_AP && static_cast<int32_t>(now - nextAttemptAt_) >= 0) {
    startStationAttempt();
  }
}

void WiFiService::credentialsChanged() {
  WiFi.disconnect(false);
  if (config_->configured()) startStationAttempt();
  else startFallbackAp();
}

const char* WiFiService::stateName() const {
  switch (state_) {
    case State::IDLE: return "IDLE";
    case State::CONNECTING: return "CONNECTING";
    case State::CONNECTED: return "CONNECTED";
    case State::FALLBACK_AP: return "FALLBACK_AP";
  }
  return "UNKNOWN";
}

String WiFiService::stationIp() const {
  return WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : String();
}

String WiFiService::apIp() const {
  return apActive_ ? WiFi.softAPIP().toString() : String();
}

