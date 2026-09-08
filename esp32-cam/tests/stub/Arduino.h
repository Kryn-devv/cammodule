/*
 * Host stub for Arduino.h — just enough for motion.h to compile off-target.
 *
 * These stubs exist so the tests exercise the REAL motion.h, rather than a
 * Python transliteration of it that could quietly drift out of step with the
 * firmware it is supposed to be checking.
 */
#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>

// On the ESP32 this allocates from external PSRAM; on the host, heap is heap.
static inline void* ps_malloc(size_t size) { return malloc(size); }
