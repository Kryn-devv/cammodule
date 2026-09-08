/*
 * Definitions for the host stubs.
 *
 * Every one of these is a no-op that returns a plausible value. The point is
 * not to simulate an ESP32 — it is to let the compiler check the ~1000 lines
 * of robot_eye.ino for the errors a careful reading misses: a misspelt sensor
 * field, a printf argument that does not match its format, an enum member
 * that does not exist, a call with the wrong number of arguments.
 */

#include "Arduino.h"
#include "ESPmDNS.h"
#include "WiFi.h"
#include "esp_camera.h"
#include "esp_http_server.h"
#include "img_converters.h"

HardwareSerial Serial;
EspClass ESP;
WiFiClass WiFi;
MDNSResponder MDNS;

// -- the fake decoder the motion tests drive ------------------------------
const uint16_t* g_fakePixels = nullptr;
int             g_fakeW = 0;
int             g_fakeH = 0;
bool            g_fakeDecodeOk = true;
int             g_decodeCalls = 0;
jpg_scale_t     g_lastScale = JPG_SCALE_NONE;

bool jpg2rgb565(const uint8_t* src, size_t src_len, uint8_t* out, jpg_scale_t scale) {
  (void)src; (void)src_len;
  g_decodeCalls++;
  g_lastScale = scale;
  if (!g_fakeDecodeOk || g_fakePixels == nullptr) return false;
  memcpy(out, g_fakePixels, (size_t)g_fakeW * (size_t)g_fakeH * 2);
  return true;
}

bool frame2jpg(camera_fb_t*, uint8_t, uint8_t** out, size_t* out_len) {
  *out = nullptr;
  *out_len = 0;
  return false;
}

// -- camera ---------------------------------------------------------------
static sensor_t g_sensor;

esp_err_t esp_camera_init(const camera_config_t*) { return ESP_OK; }
esp_err_t esp_camera_deinit() { return ESP_OK; }
camera_fb_t* esp_camera_fb_get() { return nullptr; }
void esp_camera_fb_return(camera_fb_t*) {}
sensor_t* esp_camera_sensor_get() { return &g_sensor; }

// -- httpd ----------------------------------------------------------------
esp_err_t httpd_start(httpd_handle_t*, const httpd_config_t*) { return ESP_OK; }
esp_err_t httpd_register_uri_handler(httpd_handle_t, const httpd_uri_t*) { return ESP_OK; }
esp_err_t httpd_resp_set_type(httpd_req_t*, const char*) { return ESP_OK; }
esp_err_t httpd_resp_set_status(httpd_req_t*, const char*) { return ESP_OK; }
esp_err_t httpd_resp_set_hdr(httpd_req_t*, const char*, const char*) { return ESP_OK; }
esp_err_t httpd_resp_send(httpd_req_t*, const char*, ssize_t) { return ESP_OK; }
esp_err_t httpd_resp_sendstr(httpd_req_t*, const char*) { return ESP_OK; }
esp_err_t httpd_resp_send_chunk(httpd_req_t*, const char*, ssize_t) { return ESP_OK; }
esp_err_t httpd_resp_sendstr_chunk(httpd_req_t*, const char*) { return ESP_OK; }
esp_err_t httpd_resp_send_err(httpd_req_t*, httpd_err_code_t, const char*) { return ESP_OK; }
esp_err_t httpd_req_get_url_query_str(httpd_req_t*, char* buf, size_t) { buf[0] = '\0'; return ESP_ERR_NOT_FOUND; }
esp_err_t httpd_query_key_value(const char*, const char*, char* val, size_t) { val[0] = '\0'; return ESP_ERR_NOT_FOUND; }
esp_err_t httpd_req_get_hdr_value_str(httpd_req_t*, const char*, char* val, size_t) { val[0] = '\0'; return ESP_ERR_NOT_FOUND; }
