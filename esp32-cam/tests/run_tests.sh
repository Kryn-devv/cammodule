#!/usr/bin/env sh
#
# Build and run the firmware's host tests.
#
# motion.h is compiled here as-is — the real firmware source, against small
# stubs for Arduino.h and the camera driver — so these tests check the code
# that ships rather than a transliteration of it that could drift.
#
# Everything else in the firmware talks to hardware or the network and cannot
# be meaningfully tested off-target; the compiler is the check for that part.
set -e

DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
OUT="${TMPDIR:-/tmp}/robot_eye_test_motion"

CXX=${CXX:-g++}
"$CXX" -std=c++17 -Wall -Wextra -Werror -O1 -I"$DIR" -I"$DIR/stub" \
    -o "$OUT" "$DIR/test_motion.cpp"

"$OUT"
