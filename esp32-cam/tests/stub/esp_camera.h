// Host stub for esp_camera.h — only the pieces motion.h touches.
#pragma once

#include <cstddef>
#include <cstdint>

typedef enum {
  PIXFORMAT_RGB565 = 0,
  PIXFORMAT_YUV422,
  PIXFORMAT_GRAYSCALE,
  PIXFORMAT_JPEG,
  PIXFORMAT_RGB888,
} pixformat_t;

typedef struct {
  uint8_t*    buf;
  size_t      len;
  size_t      width;
  size_t      height;
  pixformat_t format;
} camera_fb_t;
