// Host stub for ESPmDNS.h.
#pragma once

#include "Arduino.h"

class MDNSResponder {
 public:
  bool begin(const char*) { return true; }
  void end() {}
  void addService(const char*, const char*, uint16_t) {}
};
extern MDNSResponder MDNS;
