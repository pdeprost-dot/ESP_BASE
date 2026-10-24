#pragma once

#include <Arduino.h>

namespace PlatformCompat {

uint64_t hardwareId();
uint32_t freeHeap();
bool networkConnected();
void applyHostname(const char* hostname);
const char* platformName();

}  // namespace PlatformCompat

