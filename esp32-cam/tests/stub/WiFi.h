// Host stub for WiFi.h.
#pragma once

#include "Arduino.h"

#define WIFI_STA 1
#define WL_CONNECTED 3

class IPAddress {
 public:
  String toString() const { return String("0.0.0.0"); }
};

class WiFiClass {
 public:
  void mode(int) {}
  void begin(const char*, const char*) {}
  void disconnect() {}
  int status() { return WL_CONNECTED; }
  IPAddress localIP() { return IPAddress(); }
  int RSSI() { return -50; }
  String SSID() { return String("stub-ssid"); }
  void setSleep(bool) {}
  void setAutoReconnect(bool) {}
  void persistent(bool) {}
};
extern WiFiClass WiFi;
