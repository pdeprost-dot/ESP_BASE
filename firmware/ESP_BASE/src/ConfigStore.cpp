#include "ConfigStore.h"

#if defined(ESP8266)
#include <EEPROM.h>
#elif defined(ESP32)
#include <Preferences.h>
#else
#error "No ConfigStore backend for this platform"
#endif

#include <stddef.h>
#include <string.h>

uint32_t ConfigStore::checksum(const Data& data) {
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&data);
  const size_t length = offsetof(Data, checksum);
  uint32_t hash = 2166136261UL;
  for (size_t index = 0; index < length; ++index) {
    hash ^= bytes[index];
    hash *= 16777619UL;
  }
  return hash;
}

void ConfigStore::begin() {
  configured_ = loadBackend();
  if (!configured_) data_ = Data{};
}

bool ConfigStore::saveWiFi(const char* ssid, const char* password) {
  if (!ssid || !password) return false;
  const size_t ssidLength = strlen(ssid);
  const size_t passwordLength = strlen(password);
  const bool validPasswordLength = passwordLength == 0 ||
                                   (passwordLength >= 8 && passwordLength <= 64);
  if (ssidLength == 0 || ssidLength > 32 || !validPasswordLength) return false;

  Data candidate{};
  candidate.magic = kMagic;
  candidate.version = kVersion;
  memcpy(candidate.ssid, ssid, ssidLength + 1);
  memcpy(candidate.password, password, passwordLength + 1);
  candidate.checksum = checksum(candidate);
  data_ = candidate;
  configured_ = saveBackend();
  if (!configured_) data_ = Data{};
  return configured_;
}

bool ConfigStore::clear() {
  data_ = Data{};
  configured_ = false;
  return saveBackend();
}

bool ConfigStore::loadBackend() {
#if defined(ESP8266)
  EEPROM.begin(sizeof(Data));
  EEPROM.get(0, data_);
#else
  Preferences preferences;
  preferences.begin("esp-base", true);
  const size_t read = preferences.getBytes("config", &data_, sizeof(data_));
  preferences.end();
  if (read != sizeof(data_)) return false;
#endif
  data_.ssid[sizeof(data_.ssid) - 1] = '\0';
  data_.password[sizeof(data_.password) - 1] = '\0';
  return data_.magic == kMagic && data_.version == kVersion &&
         data_.ssid[0] != '\0' && data_.checksum == checksum(data_);
}

bool ConfigStore::saveBackend() {
#if defined(ESP8266)
  EEPROM.put(0, data_);
  return EEPROM.commit();
#else
  Preferences preferences;
  preferences.begin("esp-base", false);
  const size_t written = preferences.putBytes("config", &data_, sizeof(data_));
  preferences.end();
  return written == sizeof(data_);
#endif
}

