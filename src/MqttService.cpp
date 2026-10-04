#include "MqttService.h"

#include <string.h>

namespace {
bool topicMatches(const char* filter, const char* topic) {
  while (*filter && *topic) {
    if (*filter == '#') return filter[1] == '\0';
    if (*filter == '+') {
      while (*topic && *topic != '/') ++topic;
      ++filter;
    } else {
      if (*filter++ != *topic++) return false;
    }
  }
  return (*filter == '\0' && *topic == '\0') ||
         (*filter == '#' && filter[1] == '\0') ||
         (*filter == '/' && filter[1] == '#' && filter[2] == '\0');
}
}

MqttService::MqttService() : mqtt_(network_) {}

void MqttService::begin(ConfigStore& config, const DeviceIdentity& identity,
                        LogService& logs) {
  config_ = &config;
  logs_ = &logs;
  snprintf(clientId_, sizeof(clientId_), "%s-mqtt", identity.hostname());
  network_.setTimeout(2000);
  mqtt_.setSocketTimeout(2);
  mqtt_.setBufferSize(kBufferSize);
  mqtt_.setCallback([this](char* topic, uint8_t* payload, unsigned int length) {
    handleMessage(topic, payload, length);
  });
  configurationChanged();
  logs_->add("MQTT", "Ready enabled=%s client_id=%s",
             enabled() ? "yes" : "no", clientId_);
}

void MqttService::configurationChanged() {
  disconnect();
  backoffStep_ = 0;
  nextAttemptAt_ = 0;
  lastError_ = 0;
  if (config_) mqtt_.setServer(config_->mqttBroker(), config_->mqttPort());
}

void MqttService::suspend() {
  suspended_ = true;
  disconnect();
  if (logs_) logs_->add("MQTT", "Suspended for OTA");
}

uint32_t MqttService::currentBackoff() const {
  static const uint32_t delays[] = {5000, 15000, 30000, 60000};
  return delays[backoffStep_ < 4 ? backoffStep_ : 3];
}

void MqttService::tick(bool wifiConnected, bool otaBusy) {
  if (suspended_ && !otaBusy) {
    suspended_ = false;
    nextAttemptAt_ = 0;
  }
  if (!enabled() || !config_->mqttBroker()[0] || !wifiConnected || otaBusy || suspended_) {
    if (mqtt_.connected() || network_.connected()) disconnect();
    return;
  }
  if (!mqtt_.connected()) {
    const uint32_t now = millis();
    if (static_cast<int32_t>(now - nextAttemptAt_) < 0) return;
    ++connectionAttempts_;
    const bool wasConnected = everConnected_;
    const bool ok = config_->mqttUsername()[0]
                        ? mqtt_.connect(clientId_, config_->mqttUsername(), config_->mqttPassword())
                        : mqtt_.connect(clientId_);
    if (!ok) {
      lastError_ = mqtt_.state();
      const uint32_t wait = currentBackoff();
      if (backoffStep_ < 3) ++backoffStep_;
      nextAttemptAt_ = millis() + wait;
      logs_->add("MQTT", "Connection failed state=%d retry_ms=%lu", lastError_,
                 static_cast<unsigned long>(wait));
      return;
    }
    lastError_ = 0;
    backoffStep_ = 0;
    nextAttemptAt_ = 0;
    if (everConnected_) ++reconnects_;
    everConnected_ = true;
    logs_->add("MQTT", "%s broker=%s port=%u", wasConnected ? "Reconnected" : "Connected",
               config_->mqttBroker(), config_->mqttPort());
    subscribeAll();
  }
  mqtt_.loop();
}

void MqttService::disconnect() {
  if (mqtt_.connected()) mqtt_.disconnect();
  network_.stop();
}

bool MqttService::buildTopic(const char* relativeTopic, char* output, size_t capacity,
                             bool allowWildcards) const {
  if (!config_ || !relativeTopic || !output || capacity == 0) return false;
  while (*relativeTopic == '/') ++relativeTopic;
  const size_t relativeLength = strlen(relativeTopic);
  if (relativeLength == 0 || relativeLength > kMaxRelativeTopic) return false;
  if (!allowWildcards && (strchr(relativeTopic, '+') || strchr(relativeTopic, '#'))) return false;
  for (size_t index = 0; index < relativeLength; ++index) {
    const char value = relativeTopic[index];
    if (static_cast<uint8_t>(value) < 0x20) return false;
    if (allowWildcards && (value == '+' || value == '#')) {
      const bool levelStart = index == 0 || relativeTopic[index - 1] == '/';
      const bool levelEnd = index + 1 == relativeLength || relativeTopic[index + 1] == '/';
      if (!levelStart || !levelEnd || (value == '#' && index + 1 != relativeLength)) return false;
    }
  }
  const char* root = config_->mqttRootTopic();
  size_t rootLength = strlen(root);
  while (rootLength && root[rootLength - 1] == '/') --rootLength;
  if (!rootLength) return false;
  const int written = snprintf(output, capacity, "%.*s/%s", static_cast<int>(rootLength), root,
                               relativeTopic);
  return written > 0 && static_cast<size_t>(written) < capacity;
}

bool MqttService::publish(const char* relativeTopic, const char* payload, bool retained) {
  char topic[kMaxFinalTopic + 1];
  if (!payload || !buildTopic(relativeTopic, topic, sizeof(topic), false) || !connected() ||
      strlen(topic) + strlen(payload) + 8U > kBufferSize) {
    ++publishFailures_;
    return false;
  }
  const bool ok = mqtt_.publish(topic, payload, retained);
  if (ok) ++publications_; else ++publishFailures_;
  return ok;
}

bool MqttService::subscribe(const char* relativeTopic, MqttMessageHandler handler, void* context) {
  while (relativeTopic && *relativeTopic == '/') ++relativeTopic;
  char topic[kMaxFinalTopic + 1];
  if (!handler || !buildTopic(relativeTopic, topic, sizeof(topic), true)) return false;
  for (size_t index = 0; index < subscriptionCount_; ++index) {
    if (strcmp(subscriptions_[index].relativeTopic, relativeTopic) == 0) return false;
  }
  if (subscriptionCount_ >= kMaxSubscriptions) return false;
  Subscription& entry = subscriptions_[subscriptionCount_++];
  strlcpy(entry.relativeTopic, relativeTopic, sizeof(entry.relativeTopic));
  entry.handler = handler;
  entry.context = context;
  if (connected() && !mqtt_.subscribe(topic)) {
    --subscriptionCount_;
    entry = Subscription{};
    return false;
  }
  return true;
}

void MqttService::subscribeAll() {
  for (size_t index = 0; index < subscriptionCount_; ++index) {
    char topic[kMaxFinalTopic + 1];
    if (!buildTopic(subscriptions_[index].relativeTopic, topic, sizeof(topic), true) ||
        !mqtt_.subscribe(topic)) {
      logs_->add("MQTT", "Subscription failed index=%u", static_cast<unsigned>(index));
    }
  }
}

void MqttService::handleMessage(char* topic, uint8_t* payload, unsigned int length) {
  ++messagesReceived_;
  const char* relative = topic;
  const size_t rootLength = strlen(config_->mqttRootTopic());
  if (strncmp(topic, config_->mqttRootTopic(), rootLength) == 0 && topic[rootLength] == '/') {
    relative = topic + rootLength + 1;
  }
  for (size_t index = 0; index < subscriptionCount_; ++index) {
    if (topicMatches(subscriptions_[index].relativeTopic, relative)) {
      subscriptions_[index].handler(relative, payload, length, subscriptions_[index].context);
    }
  }
}
