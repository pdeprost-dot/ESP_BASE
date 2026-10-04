#pragma once

#include <Arduino.h>
#include <PubSubClient.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

#include "ConfigStore.h"
#include "DeviceIdentity.h"
#include "LogService.h"

using MqttMessageHandler = void (*)(const char* topic, const uint8_t* payload,
                                    size_t length, void* context);

class MqttService {
 public:
  MqttService();
  void begin(ConfigStore& config, const DeviceIdentity& identity, LogService& logs);
  void tick(bool wifiConnected, bool otaBusy);
  void suspend();
  void configurationChanged();

  bool publish(const char* relativeTopic, const char* payload, bool retained = false);
  bool subscribe(const char* relativeTopic, MqttMessageHandler handler,
                 void* context = nullptr);
  bool connected() const { return mqtt_.connected(); }
  bool enabled() const { return config_ && config_->mqttEnabled(); }
  const char* clientId() const { return clientId_; }
  int lastError() const { return lastError_; }
  uint32_t connectionAttempts() const { return connectionAttempts_; }
  uint32_t reconnects() const { return reconnects_; }
  uint32_t publications() const { return publications_; }
  uint32_t publishFailures() const { return publishFailures_; }
  uint32_t messagesReceived() const { return messagesReceived_; }

 private:
  static constexpr size_t kMaxSubscriptions = 4;
  static constexpr size_t kMaxRelativeTopic = 64;
  static constexpr size_t kMaxFinalTopic = 128;
  static constexpr uint16_t kBufferSize = 512;

  struct Subscription {
    char relativeTopic[kMaxRelativeTopic + 1]{};
    MqttMessageHandler handler = nullptr;
    void* context = nullptr;
  };

  bool buildTopic(const char* relativeTopic, char* output, size_t capacity,
                  bool allowWildcards) const;
  void disconnect();
  void handleMessage(char* topic, uint8_t* payload, unsigned int length);
  void subscribeAll();
  uint32_t currentBackoff() const;

  WiFiClient network_;
  mutable PubSubClient mqtt_;
  ConfigStore* config_ = nullptr;
  LogService* logs_ = nullptr;
  Subscription subscriptions_[kMaxSubscriptions];
  size_t subscriptionCount_ = 0;
  char clientId_[48]{};
  uint32_t nextAttemptAt_ = 0;
  uint8_t backoffStep_ = 0;
  int lastError_ = 0;
  uint32_t connectionAttempts_ = 0;
  uint32_t reconnects_ = 0;
  uint32_t publications_ = 0;
  uint32_t publishFailures_ = 0;
  uint32_t messagesReceived_ = 0;
  bool everConnected_ = false;
  bool suspended_ = false;
};
