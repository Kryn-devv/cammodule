/*
 * test_motion.cpp — host tests for the real motion.h.
 *
 * motion.h is the only firmware file with logic worth being wrong about: a
 * grid reduction, a frame comparison, per-mille box maths and a centroid
 * mapped into the S3 node's look coordinates. All of it is pure arithmetic
 * over the decoded pixels, so with a stub decoder it runs on the host and the
 * firmware's actual source gets checked rather than a copy of it.
 *
 * Build and run:  esp32-cam/tests/run_tests.sh
 */

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "stub/img_converters.h"

// motion.h reads these; they are the firmware's config.h values.
#define MOTION_CELL_DELTA_DEFAULT 18
#define MOTION_MIN_PERCENT_DEFAULT 2

#include "../robot_eye/motion.h"

/* ─────────────────────────── stub decoder ─────────────────────────── */

const uint16_t* g_fakePixels = nullptr;
int             g_fakeW = 0;
int             g_fakeH = 0;
bool            g_fakeDecodeOk = true;
int             g_decodeCalls = 0;
jpg_scale_t     g_lastScale = JPG_SCALE_NONE;

bool jpg2rgb565(const uint8_t* src, size_t src_len, uint8_t* out, jpg_scale_t scale) {
  (void)src;
  (void)src_len;
  g_decodeCalls++;
  g_lastScale = scale;
  if (!g_fakeDecodeOk || g_fakePixels == nullptr) return false;
  memcpy(out, g_fakePixels, (size_t)g_fakeW * (size_t)g_fakeH * 2);
  return true;
}

/* ─────────────────────────── tiny test harness ─────────────────────────── */

static int g_checks = 0;
static int g_failures = 0;
static std::string g_currentTest;

static void check(bool ok, const char* what, const std::string& detail = "") {
  g_checks++;
  if (!ok) {
    g_failures++;
    printf("  FAIL  [%s] %s", g_currentTest.c_str(), what);
    if (!detail.empty()) printf("  (%s)", detail.c_str());
    printf("\n");
  }
}

static void checkEq(long got, long want, const char* what) {
  check(got == want, what, "got " + std::to_string(got) + ", want " + std::to_string(want));
}

static void checkNear(long got, long want, long tol, const char* what) {
  check(std::labs(got - want) <= tol, what,
        "got " + std::to_string(got) + ", want " + std::to_string(want) +
            " +/- " + std::to_string(tol));
}

#define TEST(name)                     \
  do {                                 \
    g_currentTest = name;              \
    printf("- %s\n", name);            \
  } while (0)

/* ─────────────────────────── frame builders ─────────────────────────── */

// The decoder output is 1/8 of the sensor frame, so an SVGA (800x600) frame
// decodes to 100x75 — the dimensions motion.h computes for itself.
static const int SRC_W = 800;
static const int SRC_H = 600;
static const int DEC_W = SRC_W / 8;   // 100
static const int DEC_H = SRC_H / 8;   // 75

static uint16_t rgb565(int r8, int g8, int b8) {
  return (uint16_t)(((r8 & 0xF8) << 8) | ((g8 & 0xFC) << 3) | (b8 >> 3));
}

// A uniform grey field. `level` is 0..255 applied to all three channels.
static std::vector<uint16_t> flatFrame(int level, int w = DEC_W, int h = DEC_H) {
  return std::vector<uint16_t>((size_t)w * h, rgb565(level, level, level));
}

// Paint a rectangle in decoded-pixel coordinates.
static void paint(std::vector<uint16_t>& px, int x0, int y0, int x1, int y1, int level,
                  int w = DEC_W, int h = DEC_H) {
  for (int y = y0; y < y1 && y < h; y++) {
    for (int x = x0; x < x1 && x < w; x++) {
      px[(size_t)y * w + x] = rgb565(level, level, level);
    }
  }
}

static camera_fb_t makeFb(int w = SRC_W, int h = SRC_H, pixformat_t fmt = PIXFORMAT_JPEG) {
  static uint8_t dummy[4] = {0xFF, 0xD8, 0xFF, 0xD9};
  camera_fb_t fb;
  fb.buf = dummy;
  fb.len = sizeof(dummy);
  fb.width = (size_t)w;
  fb.height = (size_t)h;
  fb.format = fmt;
  return fb;
}

// Feed one frame. Returns whether the detector accepted the sample.
static bool feed(MotionDetector& d, const std::vector<uint16_t>& px, unsigned long nowMs,
                 int w = DEC_W, int h = DEC_H, int srcW = SRC_W, int srcH = SRC_H,
                 pixformat_t fmt = PIXFORMAT_JPEG,
                 uint8_t cellDelta = MOTION_CELL_DELTA_DEFAULT,
                 uint8_t minPercent = MOTION_MIN_PERCENT_DEFAULT) {
  g_fakePixels = px.data();
  g_fakeW = w;
  g_fakeH = h;
  camera_fb_t fb = makeFb(srcW, srcH, fmt);
  return d.sample(&fb, nowMs, cellDelta, minPercent);
}

/* ─────────────────────────── the tests ─────────────────────────── */

static void testBaselineFrameIsNeverAnEvent() {
  TEST("first frame is a baseline, not motion");
  MotionDetector d;
  const auto grey = flatFrame(120);

  check(feed(d, grey, 1000), "first sample accepted");
  const MotionState& m = d.state();
  check(!m.moved, "no motion on the very first frame");
  checkEq(m.changedCells, 0, "no changed cells recorded");
  checkEq(m.samples, 1, "sample counted");
  checkEq(m.events, 0, "no event raised");
  check(!m.haveBox, "no box on a baseline frame");
  checkEq(m.lastMotionMs, 0, "no motion timestamp yet");
}

static void testIdenticalFramesAreStill() {
  TEST("an unchanging scene reports no motion");
  MotionDetector d;
  const auto grey = flatFrame(120);

  feed(d, grey, 1000);
  check(feed(d, grey, 1400), "second sample accepted");
  const MotionState& m = d.state();
  check(!m.moved, "still scene is still");
  checkEq(m.changedCells, 0, "zero cells changed");
  checkEq(m.percent, 0, "zero percent");
  checkEq(m.events, 0, "no event");
  checkEq(m.samples, 2, "two samples counted");
}

static void testDecodesAtEighthScale() {
  TEST("the decoder is asked for 1/8 scale, not a full decode");
  MotionDetector d;
  const auto grey = flatFrame(120);
  g_decodeCalls = 0;
  feed(d, grey, 1000);
  checkEq(g_decodeCalls, 1, "one decode per sample");
  check(g_lastScale == JPG_SCALE_8X, "requested JPG_SCALE_8X");
}

static void testWholeFrameChangeFillsTheBox() {
  TEST("a whole-frame change reports the whole frame");
  MotionDetector d;
  feed(d, flatFrame(40), 1000);
  check(feed(d, flatFrame(220), 1400), "sample accepted");

  const MotionState& m = d.state();
  check(m.moved, "motion detected");
  checkEq(m.changedCells, MOTION_CELLS, "every cell changed");
  checkEq(m.percent, 100, "100 percent");
  check(m.haveBox, "a box was produced");
  checkEq(m.boxX, 0, "box starts at the left edge");
  checkEq(m.boxY, 0, "box starts at the top edge");
  checkEq(m.boxW, 1000, "box spans the full width in per-mille");
  checkEq(m.boxH, 1000, "box spans the full height in per-mille");
  // Centroid of a uniform change is the middle of the frame.
  // Exactly centred: the centroid maths must not truncate and drift up-left.
  checkEq(m.lookX, 0, "look x is exactly centred");
  checkEq(m.lookY, 0, "look y is exactly centred");
  checkEq(m.events, 1, "one still-to-moving event");
  checkEq(m.lastMotionMs, 1400, "motion timestamp recorded");
}

static void testChangeOnTheRightLooksRight() {
  TEST("something moving on the right gives a positive look x");
  MotionDetector d;
  const auto grey = flatFrame(60);
  feed(d, grey, 1000);

  auto moved = grey;
  paint(moved, DEC_W * 3 / 4, 0, DEC_W, DEC_H, 230);   // right quarter
  check(feed(d, moved, 1400), "sample accepted");

  const MotionState& m = d.state();
  check(m.moved, "motion detected");
  check(m.lookX > 40, "look x points right", "lookX=" + std::to_string(m.lookX));
  checkNear(m.lookY, 0, 6, "look y stays centred");
  check(m.boxX > 600, "box starts in the right part of the frame",
        "boxX=" + std::to_string(m.boxX));
  checkNear(m.boxY, 0, 45, "box covers the full height, so starts at the top");
}

static void testChangeTopLeftLooksUpAndLeft() {
  TEST("something in the top-left gives negative look x and y");
  MotionDetector d;
  const auto grey = flatFrame(60);
  feed(d, grey, 1000);

  auto moved = grey;
  paint(moved, 0, 0, DEC_W / 4, DEC_H / 4, 230);
  check(feed(d, moved, 1400), "sample accepted");

  const MotionState& m = d.state();
  check(m.moved, "motion detected");
  check(m.lookX < -40, "look x points left", "lookX=" + std::to_string(m.lookX));
  check(m.lookY < -40, "look y points up", "lookY=" + std::to_string(m.lookY));
  checkNear(m.boxX, 0, 40, "box hugs the left edge");
  checkNear(m.boxY, 0, 45, "box hugs the top edge");
  check(m.boxW < 400, "box is about a quarter wide", "boxW=" + std::to_string(m.boxW));
  check(m.boxH < 400, "box is about a quarter tall", "boxH=" + std::to_string(m.boxH));
}

static void testLookCoordinatesStayInRange() {
  TEST("look coordinates never leave the -100..100 the S3 node accepts");
  MotionDetector d;
  const auto grey = flatFrame(60);

  // Sweep a bright block across every corner and edge and check the bounds.
  const int steps = 8;
  for (int sy = 0; sy < steps; sy++) {
    for (int sx = 0; sx < steps; sx++) {
      d.resetBaseline();
      feed(d, grey, 1000);
      auto moved = grey;
      const int x0 = (DEC_W * sx) / steps;
      const int y0 = (DEC_H * sy) / steps;
      paint(moved, x0, y0, x0 + DEC_W / steps, y0 + DEC_H / steps, 240);
      feed(d, moved, 1400);
      const MotionState& m = d.state();
      if (!m.moved) continue;
      check(m.lookX >= -100 && m.lookX <= 100, "look x in range",
            "lookX=" + std::to_string(m.lookX));
      check(m.lookY >= -100 && m.lookY <= 100, "look y in range",
            "lookY=" + std::to_string(m.lookY));
      check(m.boxX <= 1000 && m.boxY <= 1000, "box origin in per-mille range");
      check((long)m.boxX + m.boxW <= 1000, "box never runs past the right edge",
            "boxX=" + std::to_string(m.boxX) + " boxW=" + std::to_string(m.boxW));
      check((long)m.boxY + m.boxH <= 1000, "box never runs past the bottom edge",
            "boxY=" + std::to_string(m.boxY) + " boxH=" + std::to_string(m.boxH));
    }
  }
}

static void testSmallChangeIsBelowThreshold() {
  TEST("a change too small for the threshold counts cells but reports no motion");
  MotionDetector d;
  const auto grey = flatFrame(60);
  feed(d, grey, 1000);

  // One grid cell is 1/768 of the frame, well under the 2% floor.
  auto moved = grey;
  const int cellW = DEC_W / MOTION_GRID_W;   // 3 px
  const int cellH = DEC_H / MOTION_GRID_H;   // 3 px
  paint(moved, 0, 0, cellW, cellH, 240);
  check(feed(d, moved, 1400), "sample accepted");

  const MotionState& m = d.state();
  check(m.changedCells >= 1, "the cell was seen to change",
        "changed=" + std::to_string(m.changedCells));
  check(!m.moved, "but it is under the minimum percentage");
  check(!m.haveBox, "no box when it does not count as motion");
  checkEq(m.events, 0, "no event raised");
  checkEq(m.lastMotionMs, 0, "no motion timestamp");
}

static void testBrightnessStepBelowCellDeltaIsIgnored() {
  TEST("a brightness step smaller than the cell delta is ignored entirely");
  MotionDetector d;
  feed(d, flatFrame(100), 1000);
  // A 5-level step, well under the default delta of 18.
  check(feed(d, flatFrame(105), 1400), "sample accepted");
  const MotionState& m = d.state();
  checkEq(m.changedCells, 0, "no cell crossed the delta");
  check(!m.moved, "no motion");
}

static void testEventsCountTransitionsNotFrames() {
  TEST("events count still-to-moving transitions, not moving frames");
  MotionDetector d;
  feed(d, flatFrame(40), 1000);

  const auto bright = flatFrame(220);
  feed(d, bright, 1400);              // still -> moving : one event
  checkEq(d.state().events, 1, "first transition counted");

  feed(d, bright, 1800);              // moving, but nothing changed: still
  check(!d.state().moved, "an unchanging bright frame is not motion");
  checkEq(d.state().events, 1, "no second event from a static frame");

  feed(d, flatFrame(40), 2200);       // changed back : another transition
  checkEq(d.state().events, 2, "second transition counted");
}

static void testRecentFlagHoldsThenExpires() {
  TEST("the recent flag holds for the hold time, then expires");
  MotionDetector d;
  feed(d, flatFrame(40), 1000);
  feed(d, flatFrame(220), 2000);
  check(d.state().moved, "motion detected at t=2000");

  d.tick(2500, 3000);
  check(d.state().recent, "still recent 500 ms later");
  d.tick(4900, 3000);
  check(d.state().recent, "still recent just inside the window");
  d.tick(5100, 3000);
  check(!d.state().recent, "expired just outside the window");
}

static void testRecentIsFalseBeforeAnyMotion() {
  TEST("recent is false before anything has ever moved");
  MotionDetector d;
  d.tick(50000, 3000);
  check(!d.state().recent, "no motion ever means never recent");
  feed(d, flatFrame(120), 1000);
  d.tick(1100, 3000);
  check(!d.state().recent, "a baseline frame does not make it recent");
}

static void testResetBaselineReBaselines() {
  TEST("resetBaseline makes the next frame a baseline again");
  MotionDetector d;
  feed(d, flatFrame(40), 1000);
  feed(d, flatFrame(220), 1400);
  check(d.state().moved, "motion detected before the reset");

  d.resetBaseline();
  check(!d.state().moved, "reset clears the moved flag");
  checkEq(d.state().changedCells, 0, "reset clears the changed count");
  check(!d.state().haveBox, "reset clears the box");

  // A completely different frame right after a reset must not be an event:
  // this is what stops a framesize change or the flash coming on from being
  // reported as somebody walking in.
  feed(d, flatFrame(40), 1800);
  check(!d.state().moved, "the frame after a reset is a baseline");
  checkEq(d.state().events, 1, "still just the one event from before");
}

static void testRejectsNonJpegAndTinyFrames() {
  TEST("frames the detector cannot use are refused, not guessed at");
  MotionDetector d;
  const auto grey = flatFrame(120);

  check(!feed(d, grey, 1000, DEC_W, DEC_H, SRC_W, SRC_H, PIXFORMAT_RGB565),
        "a non-JPEG frame is refused");
  checkEq(d.state().samples, 0, "a refused frame is not counted as a sample");

  check(!d.sample(nullptr, 1000, 18, 2), "a null frame is refused");

  // QQVGA decodes to 20x15, fewer pixels than the 32x24 grid has cells.
  const auto tiny = flatFrame(120, 20, 15);
  check(!feed(d, tiny, 1000, 20, 15, 160, 120), "a frame below the grid size is refused");
  checkEq(d.state().samples, 0, "still no samples counted");
}

static void testDecodeFailureIsNotAnEvent() {
  TEST("a failed decode changes nothing");
  MotionDetector d;
  feed(d, flatFrame(40), 1000);
  feed(d, flatFrame(220), 1400);
  const uint32_t samplesBefore = d.state().samples;
  const uint32_t eventsBefore = d.state().events;

  g_fakeDecodeOk = false;
  check(!feed(d, flatFrame(40), 1800), "the sample is refused");
  g_fakeDecodeOk = true;

  checkEq(d.state().samples, samplesBefore, "no sample counted");
  checkEq(d.state().events, eventsBefore, "no event raised");
}

static void testWorksAtEveryFrameSizeAtOrAboveQvga() {
  TEST("the fixed grid works at every framesize from QVGA up");
  struct Size { const char* name; int w, h; };
  const Size sizes[] = {
      {"qvga", 320, 240}, {"cif", 400, 296}, {"vga", 640, 480},
      {"svga", 800, 600}, {"xga", 1024, 768}, {"sxga", 1280, 1024},
      {"uxga", 1600, 1200},
  };

  for (const auto& s : sizes) {
    const int dw = s.w / 8;
    const int dh = s.h / 8;
    MotionDetector d;
    const auto grey = flatFrame(60, dw, dh);
    check(feed(d, grey, 1000, dw, dh, s.w, s.h), std::string("baseline at ").append(s.name).c_str());

    auto moved = grey;
    paint(moved, 0, 0, dw, dh, 220, dw, dh);
    check(feed(d, moved, 1400, dw, dh, s.w, s.h), std::string("change at ").append(s.name).c_str());

    const MotionState& m = d.state();
    check(m.moved, std::string("motion seen at ").append(s.name).c_str());
    checkEq(m.changedCells, MOTION_CELLS, "the whole grid changed");
    checkEq(m.boxW, 1000, "box spans the width");
    checkEq(m.boxH, 1000, "box spans the height");
  }
}

static void testSurvivesAFrameSizeChangeMidStream() {
  TEST("the grid stays comparable across a framesize change");
  MotionDetector d;
  // Baseline at SVGA...
  feed(d, flatFrame(100), 1000);
  // ...then the same scene at XGA. Same brightness, different pixel count:
  // because the grid is a fixed 32x24 either way, this must not read as motion.
  const int dw = 1024 / 8, dh = 768 / 8;
  check(feed(d, flatFrame(100, dw, dh), 1400, dw, dh, 1024, 768), "sample accepted");
  const MotionState& m = d.state();
  checkEq(m.changedCells, 0, "a resolution change alone is not motion");
  check(!m.moved, "no motion reported");
}

static void testThresholdsAreHonoured() {
  TEST("cell delta and minimum percent are actually applied");
  // Half the frame changes by 30 levels.
  auto build = [](int leftLevel, int rightLevel) {
    auto px = flatFrame(leftLevel);
    paint(px, DEC_W / 2, 0, DEC_W, DEC_H, rightLevel);
    return px;
  };

  {
    // A delta of 40 is above the 30-level step, so nothing should register.
    MotionDetector d;
    feed(d, build(100, 100), 1000, DEC_W, DEC_H, SRC_W, SRC_H, PIXFORMAT_JPEG, 40, 2);
    feed(d, build(100, 130), 1400, DEC_W, DEC_H, SRC_W, SRC_H, PIXFORMAT_JPEG, 40, 2);
    checkEq(d.state().changedCells, 0, "a step under the delta is invisible");
  }
  {
    // A delta of 10 is below it, so half the grid should register...
    MotionDetector d;
    feed(d, build(100, 100), 1000, DEC_W, DEC_H, SRC_W, SRC_H, PIXFORMAT_JPEG, 10, 2);
    feed(d, build(100, 130), 1400, DEC_W, DEC_H, SRC_W, SRC_H, PIXFORMAT_JPEG, 10, 2);
    checkNear(d.state().changedCells, MOTION_CELLS / 2, MOTION_GRID_H,
              "about half the grid changed");
    check(d.state().moved, "half the frame is well over 2 percent");
  }
  {
    // ...but a 60% minimum should refuse to call half a frame motion.
    MotionDetector d;
    feed(d, build(100, 100), 1000, DEC_W, DEC_H, SRC_W, SRC_H, PIXFORMAT_JPEG, 10, 60);
    feed(d, build(100, 130), 1400, DEC_W, DEC_H, SRC_W, SRC_H, PIXFORMAT_JPEG, 10, 60);
    check(!d.state().moved, "half a frame is under a 60 percent floor");
    check(d.state().changedCells > 0, "the cells were still counted");
  }
}

static void testPercentMatchesChangedCells() {
  TEST("the reported percentage matches the changed cell count");
  for (int fraction = 1; fraction <= 8; fraction++) {
    MotionDetector d;
    const auto grey = flatFrame(60);
    feed(d, grey, 1000);
    auto moved = grey;
    paint(moved, 0, 0, DEC_W, (DEC_H * fraction) / 8, 230);
    feed(d, moved, 1400);

    const MotionState& m = d.state();
    const long expected = (m.changedCells * 100L) / MOTION_CELLS;
    checkEq(m.percent, expected, "percent is derived from the cell count");
  }
}

static void testMillisWrapDoesNotStrandRecent() {
  TEST("the recent flag survives the millis() wrap after 49.7 days");
  MotionDetector d;
  const unsigned long nearMax = 0xFFFFFF00UL;   // ~256 ms before the wrap
  feed(d, flatFrame(40), nearMax);
  feed(d, flatFrame(220), nearMax + 100);
  check(d.state().moved, "motion seen just before the wrap");

  // 200 ms after the motion, but the counter has rolled past zero. Unsigned
  // subtraction wraps the same way, so the window must still be open.
  d.tick(nearMax + 300, 3000);
  check(d.state().recent, "still recent across the wrap");
  d.tick(nearMax + 100 + 4000, 3000);
  check(!d.state().recent, "and expires correctly on the far side");
}

int main() {
  printf("motion.h host tests\n\n");

  testBaselineFrameIsNeverAnEvent();
  testIdenticalFramesAreStill();
  testDecodesAtEighthScale();
  testWholeFrameChangeFillsTheBox();
  testChangeOnTheRightLooksRight();
  testChangeTopLeftLooksUpAndLeft();
  testLookCoordinatesStayInRange();
  testSmallChangeIsBelowThreshold();
  testBrightnessStepBelowCellDeltaIsIgnored();
  testEventsCountTransitionsNotFrames();
  testRecentFlagHoldsThenExpires();
  testRecentIsFalseBeforeAnyMotion();
  testResetBaselineReBaselines();
  testRejectsNonJpegAndTinyFrames();
  testDecodeFailureIsNotAnEvent();
  testWorksAtEveryFrameSizeAtOrAboveQvga();
  testSurvivesAFrameSizeChangeMidStream();
  testThresholdsAreHonoured();
  testPercentMatchesChangedCells();
  testMillisWrapDoesNotStrandRecent();

  printf("\n%d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
