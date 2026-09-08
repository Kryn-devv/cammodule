/*
 * Host stub for Arduino.h — just enough for the firmware to compile off-target.
 *
 * These stubs exist so the tests exercise the REAL motion.h and the real
 * sketch, rather than a transliteration that could quietly drift out of step
 * with the firmware it is supposed to be checking. They are a compile-time
 * shim only: nothing here pretends to behave like the hardware.
 */
#pragma once

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

// On the ESP32 this allocates from external PSRAM; on the host, heap is heap.
static inline void* ps_malloc(size_t size) { return malloc(size); }
static inline bool psramFound() { return true; }

#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT 0

typedef uint8_t byte;

static inline void pinMode(int, int) {}
static inline void digitalWrite(int, int) {}
static inline int digitalRead(int) { return 0; }
static inline void delay(uint32_t) {}
static inline unsigned long millis() { return 0; }

class String {
 public:
  String() = default;
  String(const char* s) : value_(s ? s : "") {}
  String(const std::string& s) : value_(s) {}
  const char* c_str() const { return value_.c_str(); }
  size_t length() const { return value_.size(); }
  int indexOf(char c, int from = 0) const {
    const auto at = value_.find(c, (size_t)from);
    return at == std::string::npos ? -1 : (int)at;
  }
  String substring(int from, int to) const {
    return String(value_.substr((size_t)from, (size_t)(to - from)));
  }
  bool operator==(const char* other) const { return value_ == (other ? other : ""); }
  String operator+(const char* other) const { return String(value_ + (other ? other : "")); }

 private:
  std::string value_;
};

class HardwareSerial {
 public:
  void begin(unsigned long) {}
  void setDebugOutput(bool) {}
  void print(const char*) {}
  void println() {}
  void println(const char*) {}
  int printf(const char*, ...) { return 0; }
};
extern HardwareSerial Serial;

class EspClass {
 public:
  uint32_t getFreeHeap() { return 0; }
  uint32_t getMinFreeHeap() { return 0; }
  uint32_t getFreePsram() { return 0; }
  void restart() {}
};
extern EspClass ESP;
