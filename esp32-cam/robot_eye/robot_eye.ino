/*
 * robot_eye.ino — ESP32-CAM firmware: the robot's eye.
 *
 * Turns the AI-Thinker ESP32-CAM into a small camera server the main brain
 * (NOVA) can pull frames from over the local network:
 *
 *   GET /            tiny human-friendly info page with a live preview
 *   GET /capture     one fresh JPEG frame  <-- the brain calls this
 *   GET /status      JSON health info (camera id, resolution, RSSI, heap)
 *   GET /flash?on=1  toggle the white flash LED (if enabled in config.h)
 *   GET :81/stream   MJPEG live stream for watching in a browser
 *
 * The stream runs on its own port (81) so someone watching the live feed
 * can never block the brain from grabbing a /capture frame on port 80.
 *
 * Board setup (Arduino IDE):
 *   - Install the "esp32" boards package (Espressif Systems)
 *   - Tools > Board > "AI Thinker ESP32-CAM"
 *   - Tools > Partition Scheme > "Huge APP"
 *   - Copy config.example.h to config.h and fill in your WiFi details
 *
 * Wiring for flashing (the AI-Thinker board has no USB port):
 *   - USB-serial adapter: 5V->5V, GND->GND, TX->U0R, RX->U0T
 *   - Hold GPIO0 to GND while pressing reset to enter flash mode;
 *     disconnect GPIO0 and reset again to run.
 */

#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include <WiFi.h>
#include <ESPmDNS.h>

#include "camera_pins.h"
#include "config.h"

static httpd_handle_t controlServer = nullptr;
static httpd_handle_t streamServer = nullptr;
static bool flashOn = false;

static void statusLed(bool on) {
  // Status LED is active LOW on the AI-Thinker board.
  digitalWrite(STATUS_LED_GPIO_NUM, on ? LOW : HIGH);
}

// ---------------------------------------------------------------- camera

static bool initCamera() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    // SVGA (800x600) is a good balance: enough detail for the vision model
    // to judge ripeness, small enough (~40 KB/frame) for a smooth stream.
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12; // 0-63, lower = better quality
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 15;
    config.fb_count = 1;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return false;
  }

  // Mild tuning that helps color accuracy (matters for ripeness judgments).
  sensor_t *s = esp_camera_sensor_get();
  if (s != nullptr) {
    s->set_whitebal(s, 1);
    s->set_awb_gain(s, 1);
    s->set_exposure_ctrl(s, 1);
    s->set_gain_ctrl(s, 1);
    s->set_saturation(s, 0);
  }
  return true;
}

static const char *frameSizeName() {
  sensor_t *s = esp_camera_sensor_get();
  if (s == nullptr) return "unknown";
  switch (s->status.framesize) {
    case FRAMESIZE_VGA: return "640x480";
    case FRAMESIZE_SVGA: return "800x600";
    case FRAMESIZE_XGA: return "1024x768";
    case FRAMESIZE_UXGA: return "1600x1200";
    default: return "other";
  }
}

// ------------------------------------------------------------- handlers

// One fresh JPEG frame. This is the endpoint the brain calls when it wants
// to look at the world.
static esp_err_t captureHandler(httpd_req_t *req) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (fb == nullptr) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Frame capture failed");
    return ESP_FAIL;
  }

  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
  httpd_resp_set_hdr(req, "X-Camera-Id", CAMERA_ID);
  esp_err_t res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  return res;
}

// MJPEG stream: multipart/x-mixed-replace, one JPEG per part, forever.
static esp_err_t streamHandler(httpd_req_t *req) {
  char partHeader[128];

  esp_err_t res = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=frame");
  if (res != ESP_OK) return res;

  while (true) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb == nullptr) {
      return ESP_FAIL;
    }

    int headerLen = snprintf(partHeader, sizeof(partHeader),
                             "\r\n--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
                             fb->len);
    res = httpd_resp_send_chunk(req, partHeader, headerLen);
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
    }
    esp_camera_fb_return(fb);

    if (res != ESP_OK) {
      // Viewer closed the tab / connection dropped: just end this handler.
      return ESP_OK;
    }
  }
}

static esp_err_t statusHandler(httpd_req_t *req) {
  char json[256];
  snprintf(json, sizeof(json),
           "{\"camera\":\"%s\",\"resolution\":\"%s\",\"wifi_rssi\":%d,"
           "\"free_heap\":%u,\"uptime_s\":%llu,\"flash_on\":%s}",
           CAMERA_ID, frameSizeName(), WiFi.RSSI(),
           (unsigned)ESP.getFreeHeap(),
           (unsigned long long)(esp_timer_get_time() / 1000000ULL),
           flashOn ? "true" : "false");
  httpd_resp_set_type(req, "application/json");
  return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t flashHandler(httpd_req_t *req) {
#if ENABLE_FLASH_LED
  char query[32] = {0};
  char value[8] = {0};
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK &&
      httpd_query_key_value(query, "on", value, sizeof(value)) == ESP_OK) {
    flashOn = (value[0] == '1');
    digitalWrite(FLASH_LED_GPIO_NUM, flashOn ? HIGH : LOW);
  }
  httpd_resp_set_type(req, "application/json");
  return httpd_resp_send(req, flashOn ? "{\"flash_on\":true}" : "{\"flash_on\":false}",
                         HTTPD_RESP_USE_STRLEN);
#else
  httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Flash LED disabled in config.h");
  return ESP_FAIL;
#endif
}

// Tiny info page with a live preview, handy for checking the eye by hand.
static esp_err_t indexHandler(httpd_req_t *req) {
  char page[640];
  snprintf(page, sizeof(page),
           "<!doctype html><title>%s</title>"
           "<body style=\"font-family:sans-serif;background:#111;color:#eee;text-align:center\">"
           "<h2>robot eye: %s</h2>"
           "<img src=\"http://%s:%d/stream\" style=\"max-width:95%%;border-radius:8px\">"
           "<p><a style=\"color:#8cf\" href=\"/capture\">/capture</a> &middot; "
           "<a style=\"color:#8cf\" href=\"/status\">/status</a></p>"
           "</body>",
           CAMERA_ID, CAMERA_ID, WiFi.localIP().toString().c_str(), STREAM_PORT);
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, page, HTTPD_RESP_USE_STRLEN);
}

// -------------------------------------------------------------- servers

static void startServers() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = HTTP_PORT;
  config.ctrl_port = 32768;

  httpd_uri_t indexUri = {.uri = "/", .method = HTTP_GET, .handler = indexHandler, .user_ctx = nullptr};
  httpd_uri_t captureUri = {.uri = "/capture", .method = HTTP_GET, .handler = captureHandler, .user_ctx = nullptr};
  httpd_uri_t statusUri = {.uri = "/status", .method = HTTP_GET, .handler = statusHandler, .user_ctx = nullptr};
  httpd_uri_t flashUri = {.uri = "/flash", .method = HTTP_GET, .handler = flashHandler, .user_ctx = nullptr};

  if (httpd_start(&controlServer, &config) == ESP_OK) {
    httpd_register_uri_handler(controlServer, &indexUri);
    httpd_register_uri_handler(controlServer, &captureUri);
    httpd_register_uri_handler(controlServer, &statusUri);
    httpd_register_uri_handler(controlServer, &flashUri);
  }

  // Separate httpd instance for the stream: its handler blocks its worker
  // thread for as long as someone is watching.
  httpd_config_t streamConfig = HTTPD_DEFAULT_CONFIG();
  streamConfig.server_port = STREAM_PORT;
  streamConfig.ctrl_port = 32769;

  httpd_uri_t streamUri = {.uri = "/stream", .method = HTTP_GET, .handler = streamHandler, .user_ctx = nullptr};

  if (httpd_start(&streamServer, &streamConfig) == ESP_OK) {
    httpd_register_uri_handler(streamServer, &streamUri);
  }
}

// ----------------------------------------------------------------- wifi

static void connectWiFi() {
  Serial.printf("Connecting to WiFi \"%s\"", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false); // keeps frame latency low
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    statusLed(true);
    delay(250);
    statusLed(false);
    delay(250);
    Serial.print(".");
  }
  Serial.printf("\nWiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());
  statusLed(true);
}

// ---------------------------------------------------------------- setup

void setup() {
  Serial.begin(115200);
  Serial.println("\n== robot_eye starting ==");

  pinMode(STATUS_LED_GPIO_NUM, OUTPUT);
  pinMode(FLASH_LED_GPIO_NUM, OUTPUT);
  digitalWrite(FLASH_LED_GPIO_NUM, LOW);

  if (!initCamera()) {
    // Camera hardware failure: blink fast forever so it's visible on the robot.
    while (true) {
      statusLed(true);
      delay(100);
      statusLed(false);
      delay(100);
    }
  }

  connectWiFi();

  if (MDNS.begin(MDNS_NAME)) {
    MDNS.addService("http", "tcp", HTTP_PORT);
    Serial.printf("mDNS: http://%s.local\n", MDNS_NAME);
  } else {
    Serial.println("mDNS start failed (camera still reachable by IP)");
  }

  startServers();

  Serial.printf("Live view:  http://%s:%d/  (stream on :%d/stream)\n",
                WiFi.localIP().toString().c_str(), HTTP_PORT, STREAM_PORT);
  Serial.printf("Brain grabs frames from:  http://%s:%d/capture\n",
                WiFi.localIP().toString().c_str(), HTTP_PORT);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi dropped, reconnecting...");
    statusLed(false);
    connectWiFi();
  }
  delay(1000);
}
