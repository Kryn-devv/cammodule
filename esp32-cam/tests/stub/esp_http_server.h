// Host stub for esp_http_server.h — the pieces the firmware touches.
#pragma once

#include <cstddef>
#include <cstdint>
#include <sys/types.h>   // ssize_t, which the httpd send functions take

#include "esp_camera.h"   // esp_err_t and the ESP_* codes

#define HTTPD_RESP_USE_STRLEN ((ssize_t)-1)

typedef enum { HTTP_GET = 1, HTTP_POST, HTTP_PUT, HTTP_DELETE } httpd_method_t;

typedef enum {
  HTTPD_500_INTERNAL_SERVER_ERROR = 0,
  HTTPD_501_METHOD_NOT_IMPLEMENTED,
  HTTPD_505_VERSION_NOT_SUPPORTED,
  HTTPD_400_BAD_REQUEST,
  HTTPD_401_UNAUTHORIZED,
  HTTPD_403_FORBIDDEN,
  HTTPD_404_NOT_FOUND,
  HTTPD_405_METHOD_NOT_ALLOWED,
  HTTPD_408_REQ_TIMEOUT,
  HTTPD_411_LENGTH_REQUIRED,
  HTTPD_414_URI_TOO_LONG,
  HTTPD_431_REQ_HDR_FIELDS_TOO_LARGE,
  HTTPD_ERR_CODE_MAX,
  // Note: there is deliberately no 503 here, matching ESP-IDF. The firmware
  // sets that status line by hand.
} httpd_err_code_t;

typedef void* httpd_handle_t;

typedef struct httpd_req {
  void* user_ctx;
} httpd_req_t;

typedef struct {
  const char*    uri;
  httpd_method_t method;
  esp_err_t (*handler)(httpd_req_t* r);
  void*          user_ctx;
} httpd_uri_t;

typedef struct {
  uint16_t server_port;
  uint16_t ctrl_port;
  uint16_t max_uri_handlers;
  uint16_t max_open_sockets;
  bool     lru_purge_enable;
  size_t   stack_size;
} httpd_config_t;

#define HTTPD_DEFAULT_CONFIG() { 80, 32768, 8, 7, false, 4096 }

esp_err_t httpd_start(httpd_handle_t* handle, const httpd_config_t* config);
esp_err_t httpd_register_uri_handler(httpd_handle_t handle, const httpd_uri_t* uri);
esp_err_t httpd_resp_set_type(httpd_req_t* r, const char* type);
esp_err_t httpd_resp_set_status(httpd_req_t* r, const char* status);
esp_err_t httpd_resp_set_hdr(httpd_req_t* r, const char* field, const char* value);
esp_err_t httpd_resp_send(httpd_req_t* r, const char* buf, ssize_t len);
esp_err_t httpd_resp_sendstr(httpd_req_t* r, const char* str);
esp_err_t httpd_resp_send_chunk(httpd_req_t* r, const char* buf, ssize_t len);
esp_err_t httpd_resp_sendstr_chunk(httpd_req_t* r, const char* str);
esp_err_t httpd_resp_send_err(httpd_req_t* r, httpd_err_code_t error, const char* msg);
esp_err_t httpd_req_get_url_query_str(httpd_req_t* r, char* buf, size_t len);
esp_err_t httpd_query_key_value(const char* qry, const char* key, char* val, size_t len);
esp_err_t httpd_req_get_hdr_value_str(httpd_req_t* r, const char* field, char* val, size_t len);
