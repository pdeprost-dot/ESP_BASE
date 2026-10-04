#pragma once

#include <Arduino.h>

class ConfigStore {
 public:
  void begin();
  bool configured() const { return configured(0); }
  bool configured(uint8_t slot) const;
  const char* ssid() const { return ssid(0); }
  const char* password() const { return password(0); }
  const char* ssid(uint8_t slot) const;
  const char* password(uint8_t slot) const;
  const char* apPassword() const { return data_.apPassword; }
  bool apAlwaysOn() const { return data_.apAlwaysOn != 0; }
  bool saveWiFi(const char* ssid, const char* password);
  bool saveWiFi(uint8_t slot, const char* ssid, const char* password);
  bool saveStations(const char* ssid1, const char* password1,
                    const char* ssid2, const char* password2);
  bool saveApPassword(const char* password);
  bool resetApPassword();
  bool saveApSettings(const char* password, bool alwaysOn);
  bool clear();

  static const char* defaultApPassword() { return "ESPbaseSetup"; }

 private:
  static constexpr uint32_t kMagic = 0x45535042UL;
  static constexpr uint16_t kVersion = 4;

  struct LegacyDataV1 {
    uint32_t magic;
    uint16_t version;
    char ssid[33];
    char password[65];
    uint32_t checksum;
  };

  struct LegacyDataV2 {
    uint32_t magic;
    uint16_t version;
    char ssid[33];
    char password[65];
    char adminPassword[17];
    uint32_t checksum;
  };

  struct LegacyDataV3 {
    uint32_t magic;
    uint16_t version;
    char ssid[33];
    char password[65];
    char adminPassword[17];
    char apPassword[64];
    uint32_t checksum;
  };

  struct Data {
    uint32_t magic = 0;
    uint16_t version = 0;
    char ssid1[33]{};
    char password1[65]{};
    char ssid2[33]{};
    char password2[65]{};
    char adminPassword[64]{};
    char apPassword[64]{};
    uint8_t apAlwaysOn = 0;
    uint32_t checksum = 0;
  };

  static uint32_t checksum(const Data& data);
  static uint32_t legacyV1Checksum(const LegacyDataV1& data);
  static uint32_t legacyV2Checksum(const LegacyDataV2& data);
  static uint32_t legacyV3Checksum(const LegacyDataV3& data);
  static bool validStation(const char* ssid, const char* password, bool optional);
  static bool validPassword(const char* password, bool allowEmpty);
  bool loadBackend();
  bool saveBackend();

  Data data_{};
};

