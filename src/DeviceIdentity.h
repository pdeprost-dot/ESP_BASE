#pragma once

#include <Arduino.h>

#include "PlatformCompat.h"

class DeviceIdentity {
 public:
  void begin();
  const char* id() const { return id_; }
  const char* hostname() const { return hostname_; }

 private:
  char id_[24]{};
  char hostname_[32]{};
};

