#pragma once

#include <Arduino.h>

class ConfigStore {
 public:
  void begin();
  bool configured() const { return configured_; }
  const char* ssid() const { return data_.ssid; }
  const char* password() const { return data_.password; }
  bool saveWiFi(const char* ssid, const char* password);
  bool clear();

 private:
  static constexpr uint32_t kMagic = 0x45535042UL;
  static constexpr uint16_t kVersion = 1;

  struct Data {
    uint32_t magic = 0;
    uint16_t version = 0;
    char ssid[33]{};
    char password[65]{};
    uint32_t checksum = 0;
  };

  static uint32_t checksum(const Data& data);
  bool loadBackend();
  bool saveBackend();

  Data data_{};
  bool configured_ = false;
};

