// Host stub for esp_camera.h — the pieces the firmware touches.
#pragma once

#include <cstddef>
#include <cstdint>

typedef int esp_err_t;
#ifndef ESP_OK
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_NOT_FOUND 0x105
#define ESP_ERR_HTTPD_RESULT_TRUNC 0x8003
#endif

typedef enum {
  PIXFORMAT_RGB565 = 0, PIXFORMAT_YUV422, PIXFORMAT_YUV420,
  PIXFORMAT_GRAYSCALE, PIXFORMAT_JPEG, PIXFORMAT_RGB888,
} pixformat_t;

typedef enum {
  FRAMESIZE_96X96 = 0, FRAMESIZE_QQVGA, FRAMESIZE_QCIF, FRAMESIZE_HQVGA,
  FRAMESIZE_240X240, FRAMESIZE_QVGA, FRAMESIZE_CIF, FRAMESIZE_HVGA,
  FRAMESIZE_VGA, FRAMESIZE_SVGA, FRAMESIZE_XGA, FRAMESIZE_HD,
  FRAMESIZE_SXGA, FRAMESIZE_UXGA, FRAMESIZE_INVALID,
} framesize_t;

typedef enum { CAMERA_GRAB_WHEN_EMPTY = 0, CAMERA_GRAB_LATEST } camera_grab_mode_t;
typedef enum { CAMERA_FB_IN_PSRAM = 0, CAMERA_FB_IN_DRAM } camera_fb_location_t;

#define LEDC_CHANNEL_0 0
#define LEDC_TIMER_0 0
#define OV2640_PID 0x26
#define OV3660_PID 0x3660

typedef struct {
  uint8_t*    buf;
  size_t      len;
  size_t      width;
  size_t      height;
  pixformat_t format;
} camera_fb_t;

typedef struct {
  int ledc_channel, ledc_timer;
  int pin_d0, pin_d1, pin_d2, pin_d3, pin_d4, pin_d5, pin_d6, pin_d7;
  int pin_xclk, pin_pclk, pin_vsync, pin_href, pin_sccb_sda, pin_sccb_scl;
  int pin_pwdn, pin_reset;
  int xclk_freq_hz;
  pixformat_t pixel_format;
  framesize_t frame_size;
  int jpeg_quality;
  size_t fb_count;
  camera_grab_mode_t grab_mode;
  camera_fb_location_t fb_location;
} camera_config_t;

typedef struct {
  uint16_t PID;
  uint16_t VER;
  uint16_t MIDL;
  uint16_t MIDH;
} camera_sensor_id_t;

typedef struct {
  framesize_t framesize;
  uint8_t quality, brightness, contrast, saturation, sharpness;
  uint8_t special_effect, wb_mode, awb, awb_gain;
  uint8_t aec, aec2, ae_level;
  uint16_t aec_value;
  uint8_t agc, agc_gain, gainceiling;
  uint8_t bpc, wpc, raw_gma, lenc, denoise;
  uint8_t hmirror, vflip, colorbar;
} camera_status_t;

struct sensor_t;
typedef int (*sensor_set_int_t)(struct sensor_t*, int);

typedef struct sensor_t {
  camera_sensor_id_t id;
  camera_status_t status;
  sensor_set_int_t set_framesize;
  sensor_set_int_t set_quality;
  sensor_set_int_t set_brightness;
  sensor_set_int_t set_contrast;
  sensor_set_int_t set_saturation;
  sensor_set_int_t set_sharpness;
  sensor_set_int_t set_special_effect;
  sensor_set_int_t set_wb_mode;
  sensor_set_int_t set_whitebal;
  sensor_set_int_t set_awb_gain;
  sensor_set_int_t set_exposure_ctrl;
  sensor_set_int_t set_aec2;
  sensor_set_int_t set_ae_level;
  sensor_set_int_t set_aec_value;
  sensor_set_int_t set_gain_ctrl;
  sensor_set_int_t set_agc_gain;
  sensor_set_int_t set_gainceiling;
  sensor_set_int_t set_bpc;
  sensor_set_int_t set_wpc;
  sensor_set_int_t set_raw_gma;
  sensor_set_int_t set_lenc;
  sensor_set_int_t set_denoise;
  sensor_set_int_t set_hmirror;
  sensor_set_int_t set_vflip;
  sensor_set_int_t set_colorbar;
} sensor_t;

esp_err_t     esp_camera_init(const camera_config_t* config);
esp_err_t     esp_camera_deinit();
camera_fb_t*  esp_camera_fb_get();
void          esp_camera_fb_return(camera_fb_t* fb);
sensor_t*     esp_camera_sensor_get();
