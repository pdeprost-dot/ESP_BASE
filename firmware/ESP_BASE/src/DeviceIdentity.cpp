#include "DeviceIdentity.h"

void DeviceIdentity::begin() {
  const uint32_t shortId = static_cast<uint32_t>(PlatformCompat::hardwareId() & 0xFFFFFFUL);
  snprintf(id_, sizeof(id_), "%s-%06X", PlatformCompat::platformName(), shortId);
  snprintf(hostname_, sizeof(hostname_), "espbase-%06x", shortId);
}

