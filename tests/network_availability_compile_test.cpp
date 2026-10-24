#include "../src/NetworkAvailability.h"

using ESPBaseNetwork::available;

static_assert(available(true, false, false), "ESP8266 Wi-Fi connected");
static_assert(!available(false, false, false), "ESP8266 Wi-Fi disconnected");
static_assert(available(true, false, false), "ESP32 Wi-Fi connected");
static_assert(available(false, true, true), "ESP32 Ethernet with IPv4");
static_assert(!available(false, false, true), "Ethernet initialized without link");
static_assert(!available(false, true, false), "Ethernet link without IPv4");
static_assert(!available(false, false, false), "ESP32 without network");
