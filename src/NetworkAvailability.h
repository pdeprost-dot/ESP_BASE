#pragma once

namespace ESPBaseNetwork {

constexpr bool available(bool wifiConnected, bool ethernetLink,
                         bool ethernetHasIpv4) {
  return wifiConnected || (ethernetLink && ethernetHasIpv4);
}

}  // namespace ESPBaseNetwork
