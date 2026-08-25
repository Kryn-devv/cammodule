// Pin map for the AI-Thinker ESP32-CAM board (OV2640 sensor).
// If you use a different ESP32 camera board (WROVER-KIT, ESP-EYE, M5Stack),
// replace this block with the matching pin map from the esp32-camera examples.
#pragma once

#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

// On-board white flash LED and red status LED
#define FLASH_LED_GPIO_NUM 4
#define STATUS_LED_GPIO_NUM 33 // active LOW
