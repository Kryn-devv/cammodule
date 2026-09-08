#!/usr/bin/env sh
#
# Firmware checks that run on a normal machine, no ESP32 needed.
#
#   1. Compile the whole sketch. robot_eye.ino is otherwise only ever checked
#      when you try to flash it and find out the hard way. This builds it
#      against small stubs for Arduino.h, WiFi, mDNS and the camera driver —
#      it runs nothing, it just makes the compiler read all of it. The config
#      is generated from config.example.h, so this also proves the example
#      config is a working one and never touches your real config.h.
#
#   2. Run the motion tests. motion.h is compiled as-is, so what gets checked
#      is the source that ships rather than a copy of it that could drift.
#
set -e

DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
FW="$DIR/../robot_eye"
CXX=${CXX:-g++}
FLAGS="-std=gnu++17 -Wall -Wextra -Werror -O1"

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

echo "== compiling robot_eye.ino =="
cp "$FW"/*.h "$FW"/robot_eye.ino "$WORK/"
# config.h is gitignored, so stand one up from the example.
sed 's/"your-wifi-name"/"test-ssid"/; s/"your-wifi-password"/"test-password"/' \
    "$FW/config.example.h" > "$WORK/config.h"
cp "$DIR/compile_check.cpp" "$WORK/"
# shellcheck disable=SC2086
"$CXX" $FLAGS -fsyntax-only -I"$WORK" -I"$DIR/stub" "$WORK/compile_check.cpp"
echo "   ok"

# The same again with motion detection compiled out, since config.h can turn
# it off and an #if that only builds one way round is not much of a switch.
sed -i 's/#define ENABLE_MOTION 1/#define ENABLE_MOTION 0/; s/#define ENABLE_FLASH_LED 1/#define ENABLE_FLASH_LED 0/' "$WORK/config.h"
# shellcheck disable=SC2086
"$CXX" $FLAGS -fsyntax-only -I"$WORK" -I"$DIR/stub" "$WORK/compile_check.cpp"
echo "   ok with ENABLE_MOTION=0 and ENABLE_FLASH_LED=0"

echo
echo "== motion.h tests =="
# shellcheck disable=SC2086
"$CXX" $FLAGS -I"$DIR" -I"$DIR/stub" -o "$WORK/test_motion" "$DIR/test_motion.cpp"
"$WORK/test_motion"
