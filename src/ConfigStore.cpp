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

namespace {
template <typename T>
uint32_t hashUntilChecksum(const T& data) {
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&data);
  const size_t length = offsetof(T, checksum);
  uint32_t hash = 2166136261UL;
  for (size_t index = 0; index < length; ++index) {
    hash ^= bytes[index];
    hash *= 16777619UL;
  }
  return hash;
}
}

uint32_t ConfigStore::checksum(const Data& data) { return hashUntilChecksum(data); }
uint32_t ConfigStore::legacyV1Checksum(const LegacyDataV1& data) { return hashUntilChecksum(data); }
uint32_t ConfigStore::legacyV2Checksum(const LegacyDataV2& data) { return hashUntilChecksum(data); }
uint32_t ConfigStore::legacyV3Checksum(const LegacyDataV3& data) { return hashUntilChecksum(data); }

bool ConfigStore::validPassword(const char* value, bool allowEmpty) {
  if (!value) return false;
  const size_t length = strlen(value);
  if (length == 0) return allowEmpty;
  if (length < 8 || length > 63) return false;
  for (size_t index = 0; index < length; ++index) {
    const uint8_t character = static_cast<uint8_t>(value[index]);
    if (character < 0x20 || character > 0x7e) return false;
  }
  return true;
}

bool ConfigStore::validStation(const char* ssidValue, const char* passwordValue, bool optional) {
  if (!ssidValue || !passwordValue) return false;
  const size_t ssidLength = strlen(ssidValue);
  if (ssidLength == 0) return optional && passwordValue[0] == '\0';
  return ssidLength <= 32 && validPassword(passwordValue, true);
}

void ConfigStore::begin() {
  if (!loadBackend()) data_ = Data{};
  if (data_.apPassword[0] == '\0') {
    strlcpy(data_.apPassword, defaultApPassword(), sizeof(data_.apPassword));
    data_.magic = kMagic;
    data_.version = kVersion;
    data_.checksum = checksum(data_);
    saveBackend();
  }
}

bool ConfigStore::configured(uint8_t slot) const {
  return slot < 2 && (slot == 0 ? data_.ssid1[0] : data_.ssid2[0]) != '\0';
}
const char* ConfigStore::ssid(uint8_t slot) const { return slot == 1 ? data_.ssid2 : data_.ssid1; }
const char* ConfigStore::password(uint8_t slot) const {
  return slot == 1 ? data_.password2 : data_.password1;
}

bool ConfigStore::saveStations(const char* ssid1, const char* password1,
                               const char* ssid2, const char* password2) {
  if (!validStation(ssid1, password1, false) || !validStation(ssid2, password2, true)) return false;
  Data candidate = data_;
  strlcpy(candidate.ssid1, ssid1, sizeof(candidate.ssid1));
  strlcpy(candidate.password1, password1, sizeof(candidate.password1));
  strlcpy(candidate.ssid2, ssid2, sizeof(candidate.ssid2));
  strlcpy(candidate.password2, password2, sizeof(candidate.password2));
  candidate.magic = kMagic;
  candidate.version = kVersion;
  candidate.checksum = checksum(candidate);
  data_ = candidate;
  return saveBackend();
}
bool ConfigStore::saveWiFi(const char* ssidValue, const char* passwordValue) {
  return saveWiFi(0, ssidValue, passwordValue);
}
bool ConfigStore::saveWiFi(uint8_t slot, const char* ssidValue, const char* passwordValue) {
  if (slot > 1) return false;
  return slot == 0 ? saveStations(ssidValue, passwordValue, data_.ssid2, data_.password2)
                   : saveStations(data_.ssid1, data_.password1, ssidValue, passwordValue);
}

bool ConfigStore::clear() {
  memset(data_.ssid1, 0, sizeof(data_.ssid1));
  memset(data_.password1, 0, sizeof(data_.password1));
  memset(data_.ssid2, 0, sizeof(data_.ssid2));
  memset(data_.password2, 0, sizeof(data_.password2));
  data_.magic = kMagic;
  data_.version = kVersion;
  data_.checksum = checksum(data_);
  return saveBackend();
}

bool ConfigStore::saveApSettings(const char* value, bool alwaysOn) {
  if (!validPassword(value, false)) return false;
  Data candidate = data_;
  strlcpy(candidate.apPassword, value, sizeof(candidate.apPassword));
  candidate.apAlwaysOn = alwaysOn ? 1 : 0;
  candidate.magic = kMagic;
  candidate.version = kVersion;
  candidate.checksum = checksum(candidate);
  data_ = candidate;
  return saveBackend();
}
bool ConfigStore::saveApPassword(const char* value) { return saveApSettings(value, apAlwaysOn()); }
bool ConfigStore::resetApPassword() { return saveApSettings(defaultApPassword(), apAlwaysOn()); }

bool ConfigStore::loadBackend() {
#if defined(ESP8266)
  EEPROM.begin(sizeof(Data));
  EEPROM.get(0, data_);
#else
  Preferences preferences;
  preferences.begin("esp-base", true);
  const size_t storedLength = preferences.getBytesLength("config");
  const size_t read = storedLength == sizeof(data_) ? preferences.getBytes("config", &data_, sizeof(data_)) : 0;
  LegacyDataV3 legacyV3{}; LegacyDataV2 legacyV2{}; LegacyDataV1 legacyV1{};
  const size_t readV3 = storedLength == sizeof(legacyV3) ? preferences.getBytes("config", &legacyV3, sizeof(legacyV3)) : 0;
  const size_t readV2 = storedLength == sizeof(legacyV2) ? preferences.getBytes("config", &legacyV2, sizeof(legacyV2)) : 0;
  const size_t readV1 = storedLength == sizeof(legacyV1) ? preferences.getBytes("config", &legacyV1, sizeof(legacyV1)) : 0;
  preferences.end();
  if (read != sizeof(data_)) {
    if (readV3 == sizeof(legacyV3) && legacyV3.magic == kMagic && legacyV3.version == 3 && legacyV3.checksum == legacyV3Checksum(legacyV3)) {
      memcpy(data_.ssid1, legacyV3.ssid, sizeof(data_.ssid1));
      memcpy(data_.password1, legacyV3.password, sizeof(data_.password1));
      memcpy(data_.adminPassword, legacyV3.adminPassword, sizeof(legacyV3.adminPassword));
      memcpy(data_.apPassword, legacyV3.apPassword, sizeof(data_.apPassword));
    } else if (readV2 == sizeof(legacyV2) && legacyV2.magic == kMagic && legacyV2.version == 2 && legacyV2.checksum == legacyV2Checksum(legacyV2)) {
      memcpy(data_.ssid1, legacyV2.ssid, sizeof(data_.ssid1));
      memcpy(data_.password1, legacyV2.password, sizeof(data_.password1));
      memcpy(data_.adminPassword, legacyV2.adminPassword, sizeof(legacyV2.adminPassword));
    } else if (readV1 == sizeof(legacyV1) && legacyV1.magic == kMagic && legacyV1.version == 1 && legacyV1.checksum == legacyV1Checksum(legacyV1)) {
      memcpy(data_.ssid1, legacyV1.ssid, sizeof(data_.ssid1));
      memcpy(data_.password1, legacyV1.password, sizeof(data_.password1));
    } else return false;
    if (data_.apPassword[0] == '\0') strlcpy(data_.apPassword, defaultApPassword(), sizeof(data_.apPassword));
    data_.magic = kMagic; data_.version = kVersion; data_.checksum = checksum(data_);
    return saveBackend();
  }
#endif
  data_.ssid1[sizeof(data_.ssid1) - 1] = '\0'; data_.password1[sizeof(data_.password1) - 1] = '\0';
  data_.ssid2[sizeof(data_.ssid2) - 1] = '\0'; data_.password2[sizeof(data_.password2) - 1] = '\0';
  data_.adminPassword[sizeof(data_.adminPassword) - 1] = '\0'; data_.apPassword[sizeof(data_.apPassword) - 1] = '\0';
  if (data_.magic == kMagic && data_.version == kVersion && data_.checksum == checksum(data_)) return true;
#if defined(ESP8266)
  LegacyDataV3 legacyV3{}; EEPROM.get(0, legacyV3);
  if (legacyV3.magic == kMagic && legacyV3.version == 3 && legacyV3.checksum == legacyV3Checksum(legacyV3)) {
    data_ = Data{}; memcpy(data_.ssid1, legacyV3.ssid, sizeof(data_.ssid1));
    memcpy(data_.password1, legacyV3.password, sizeof(data_.password1));
    memcpy(data_.adminPassword, legacyV3.adminPassword, sizeof(legacyV3.adminPassword));
    memcpy(data_.apPassword, legacyV3.apPassword, sizeof(data_.apPassword));
  } else {
    LegacyDataV2 legacyV2{}; EEPROM.get(0, legacyV2);
    if (legacyV2.magic == kMagic && legacyV2.version == 2 && legacyV2.checksum == legacyV2Checksum(legacyV2)) {
      data_ = Data{}; memcpy(data_.ssid1, legacyV2.ssid, sizeof(data_.ssid1));
      memcpy(data_.password1, legacyV2.password, sizeof(data_.password1));
      memcpy(data_.adminPassword, legacyV2.adminPassword, sizeof(legacyV2.adminPassword));
    } else {
      LegacyDataV1 legacyV1{}; EEPROM.get(0, legacyV1);
      if (legacyV1.magic != kMagic || legacyV1.version != 1 || legacyV1.checksum != legacyV1Checksum(legacyV1)) return false;
      data_ = Data{}; memcpy(data_.ssid1, legacyV1.ssid, sizeof(data_.ssid1));
      memcpy(data_.password1, legacyV1.password, sizeof(data_.password1));
    }
  }
  if (data_.apPassword[0] == '\0') strlcpy(data_.apPassword, defaultApPassword(), sizeof(data_.apPassword));
  data_.magic = kMagic; data_.version = kVersion; data_.checksum = checksum(data_);
  return saveBackend();
#else
  return false;
#endif
}

bool ConfigStore::saveBackend() {
#if defined(ESP8266)
  EEPROM.put(0, data_); return EEPROM.commit();
#else
  Preferences preferences; preferences.begin("esp-base", false);
  const size_t written = preferences.putBytes("config", &data_, sizeof(data_));
  preferences.end(); return written == sizeof(data_);
#endif
}
