/*
 * Compile the whole sketch on the host.
 *
 * This runs no firmware — esp_camera_fb_get() returns null and every handler
 * would bail immediately. It exists to make the compiler read robot_eye.ino,
 * which is otherwise only ever checked when you try to flash it and find out
 * the hard way.
 *
 * Built by run_tests.sh, which assembles a temporary copy of the sketch with
 * a config.h generated from config.example.h — so this also verifies that the
 * example config is a working config, and does not touch your real one.
 */

#include "robot_eye.ino"

int main() {
  // Referenced so the compiler cannot discard either as unused. Not called:
  // setup() would reboot on a camera that never initialises.
  void (*const entry_points[])() = {&setup, &loop};
  (void)entry_points;
  return 0;
}
