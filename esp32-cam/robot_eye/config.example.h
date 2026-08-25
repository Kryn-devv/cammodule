// Copy this file to config.h and fill in your own values.
// config.h is gitignored so your WiFi password never gets committed.
#pragma once

// --- WiFi ---
#define WIFI_SSID "your-wifi-name"
#define WIFI_PASSWORD "your-wifi-password"

// --- Camera identity ---
// A short name for this camera, in case the robot grows more than one eye.
#define CAMERA_ID "front-eye"

// mDNS hostname: the brain can reach the camera at http://robot-eye.local
// instead of a hardcoded IP address.
#define MDNS_NAME "robot-eye"

// --- Ports ---
// Control server: /, /capture, /status, /flash
#define HTTP_PORT 80
// MJPEG live stream runs on its own port so a viewer watching /stream
// can never block the brain from grabbing a /capture frame.
#define STREAM_PORT 81

// Set to 1 to allow the /flash endpoint to switch on the white flash LED
// (very bright, useful in the dark, eats power).
#define ENABLE_FLASH_LED 1
