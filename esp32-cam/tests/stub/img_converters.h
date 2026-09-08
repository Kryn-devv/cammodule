/*
 * Host stub for img_converters.h.
 *
 * The real jpg2rgb565 decodes a JPEG. Here the test hands over the pixels it
 * wants "decoded" and this copies them out, so motion.h's own reduce() and
 * frame-comparison code runs on precisely known input.
 */
#pragma once

#include "esp_camera.h"

typedef enum {
  JPG_SCALE_NONE = 0,
  JPG_SCALE_2X,
  JPG_SCALE_4X,
  JPG_SCALE_8X,
} jpg_scale_t;

// Set by the test before each sample().
extern const uint16_t* g_fakePixels;
extern int             g_fakeW;
extern int             g_fakeH;
extern bool            g_fakeDecodeOk;
extern int             g_decodeCalls;
extern jpg_scale_t     g_lastScale;

bool jpg2rgb565(const uint8_t* src, size_t src_len, uint8_t* out, jpg_scale_t scale);
bool frame2jpg(camera_fb_t* fb, uint8_t quality, uint8_t** out, size_t* out_len);
