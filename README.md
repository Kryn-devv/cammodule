# cammodule — the robot's eye (ESP32-CAM)

Firmware that turns an AI-Thinker **ESP32-CAM** into the eye of the IRIS /
"Friday" robot, plus the brain-side code that makes it *see*: it recognises
your face, tells you apart from anyone else it has met, and names whatever you
hold up in front of it.

Two halves:

- **`esp32-cam/robot_eye/`** — the firmware. A camera server, plus cheap
  presence detection so the brain does not have to pull frames to find out
  whether the room is empty.
- **`brain/`** — the Python side, ready to drop into
  [Iris_AI](https://github.com/Kryn-devv/Iris_AI). Face detection,
  recognition and enrolment run here; object identification goes to a vision
  model. See [`brain/INSTALL.md`](brain/INSTALL.md).

## How it attaches to the ESP32-S3

**Two wires: 5V and ground. No data pins.** They talk over WiFi.

That is not a shortcut — it is the only way round, and worth being precise
about because the obvious plan does not fit:

```
             ┌──────────────────────────┐
   5V ───────┤  ESP32-CAM (robot_eye)   │
  GND ───────┤  OV2640 + presence gate  │
      │      └───────────┬──────────────┘
      │                  │ WiFi
      │      ┌───────────┴──────────────┐        ┌─────────────────────────┐
      │      │   your WiFi network      ├────────┤  IRIS / Friday brain    │
      │      └───────────┬──────────────┘        │  (PC or VPS, Python)    │
      │                  │ WiFi                  │  • face recognition     │
      │      ┌───────────┴──────────────┐        │  • object ID (vision)   │
   5V ───────┤  ESP32-S3 (iris node)    │        │  • voice + intent       │
  GND ───────┤  eyes, sensors, mic, amp │        └─────────────────────────┘
             └──────────────────────────┘
   one 5V supply, common ground
```

### Why not wire the camera to the S3 directly

A DVP camera needs **15 pins** — eight data lines, XCLK, PCLK, VSYNC, HREF and
the two I²C lines. The S3 node firmware already has these spoken for:

| Pins | What has them |
|---|---|
| 9, 10, 11, 12 | the two OLED eyes (two I²C buses) |
| 4, 5, 6 | PIR, MQ-2 gas, LDR |
| 7, 8, 38, 39 | the two HC-SR04 distance sensors |
| 13, 40 | flame module, DHT11 |
| 14, 15, 16 | INMP441 microphone (I²S) |
| 17, 18, 21 | MAX98357A amplifier (I²S) |

That is 19 GPIOs. Of what is left on an ESP32-S3-DevKitC-1, 26–32 are the SPI
flash, 33–37 go to the octal PSRAM on the 8 MB modules, 19–20 are the native
USB pins, 43–44 are the serial monitor you flash and debug over, and 0, 3, 45
and 46 are strapping pins. Genuinely free: roughly **1, 2, 41, 42, 47, 48**.

Six pins, and a camera needs fifteen. Wiring the sensor to the S3 means giving
up an eye and most of the senses — and the ESP32-CAM board already has the
sensor wired correctly to its own chip, with PSRAM to hold the frames.

So the camera stays its own board. Mount it on the head next to the eyes, feed
it from the same 5V rail, and let the brain talk to both over WiFi. The brain
already reaches the S3 that way; the camera is just a second device.

**Power it properly.** Feed the ESP32-CAM **5V**, not the S3's 3.3V rail: it
pulls ~180 mA steady and spikes past 300 mA when the WiFi radio transmits,
which is more than a dev board's regulator will give you on top of two OLEDs
and an amplifier. Brownouts are the single most common cause of an ESP32-CAM
that reboots, refuses to start its camera, or drops frames — so `GET /status`
reports `reset_reason`, and `"brownout"` there means the supply, not the code.

### Why face recognition runs in the brain, not on the camera

Because the camera cannot do it any more:

- Espressif's face pipeline (ESP-WHO / ESP-DL) **dropped the original ESP32**.
  The current branch targets the S3 and P4; plain ESP32 support lives only in
  an old branch. ([esp-who](https://github.com/espressif/esp-who),
  [ESP-DL for ESP32-S3](https://docs.espressif.com/projects/esp-dl/en/release-v1.1/esp32s3/introduction.html))
- The Arduino ESP32 core **removed the face headers** (`fd_forward.h`,
  `fr_forward.h`, `face_recognition_112_v1_s8.hpp`) after 3.0.7.
  ([arduino-esp32#10881](https://github.com/espressif/arduino-esp32/issues/10881))
- Even while they existed, recognition on a plain ESP32 was too slow to feel
  like anything but a wait.

Pinning an old core to get them back trades every other fix and library update
for a slow, low-resolution recogniser. The brain has the CPU, the models and —
the part that matters — the memory of who you are. So the camera sends pixels.

There is a real division of labour, not just a punt:

| Question | Answered where | Why there |
|---|---|---|
| "has anything moved?" | **on the camera** | one 1/8-scale JPEG decode, ~40 ms. No frame crosses the network to learn the room is empty. |
| "who am I?" | **on your PC** | a closed comparison against faces it has been shown. Works with the internet down, and hosted vision models decline to identify named people anyway. |
| "what is this?" | **a vision model** | naming an arbitrary object is open-ended, which is exactly what those models are for. |

If you later get an **ESP32-S3** camera board (S3-EYE, XIAO ESP32S3 Sense),
on-device face detection becomes possible again — `camera_pins.h` already has
both pin maps.

## What Friday can do with it

Once installed (see [`brain/INSTALL.md`](brain/INSTALL.md)):

```
you: remember my face as Prakash
     → Got it — I'll remember that face as Prakash. Show me once or twice
       more, in different light, and I'll be steadier about it.

you: who am I
     → Yes, that's you.

you: who is that
     → There's someone there, but I don't recognise them.

you: what am I holding
     → A stainless steel water bottle with a black lid.

you: is this apple ripe
     → The apple looks ripe — deep even red, no green at the stem, skin
       still taut.

you: read this label
     → "Best before 04/2027. Contains almonds."

you: can you see anyone
     → Yes, something's moving to my left.

you: who do you know
     → I know 2 faces: Prakash (you), Aditi.
```

The distinctions in those answers are deliberate. "Nobody there", "someone too
far away to make out", "someone I don't recognise" and "that's you" are four
different states, and collapsing the middle two makes the robot look broken
when it is only looking across a room.

Ask a couple of times in different light before judging recognition — each
enrolment is stored as a separate sample and matching compares against all of
them, so three samples are much steadier than one.

## Endpoints

| Endpoint | What it does |
|---|---|
| `GET /` | info page: live preview, test buttons, live status |
| `GET /capture` | **one fresh JPEG — this is what the brain calls** |
| `GET /capture?size=xga&warmup=2` | switch resolution, drop N frames so exposure has settled |
| `GET /capture?flash=1&flash_ms=200` | pulse the white LED for this one shot |
| `GET /motion` | **"is anybody there?", and where** — cheap, no frame transferred |
| `GET /status` | camera, WiFi, heap, uptime, frame counters, reset cause |
| `GET /settings` | read every sensor setting; set them with query parameters |
| `GET /flash?on=1&ms=400` | white flash LED, optionally as a self-expiring pulse |
| `GET :81/stream` | MJPEG live stream (own port, for a browser) |

The stream runs on its own port so a browser watching the live feed can never
block the brain from grabbing a `/capture` frame.

With mDNS the camera is `http://robot-eye.local` — no hardcoded IP.

### `/motion` — presence without frames

The camera watches for change itself. Every 400 ms it decodes the current JPEG
at 1/8 scale (the decoder skips most of its work there — about 40 ms at SVGA,
against ~300 ms for a full decode), squashes it into a fixed 32×24 grid of
average brightness, and compares that with the previous one.

```json
{
  "enabled": true, "ready": true, "moved": true, "recent": true,
  "percent": 9, "changed_cells": 71, "grid": {"w": 32, "h": 24},
  "box": {"x": 218, "y": 125, "w": 375, "h": 583, "units": "per-mille"},
  "look": {"x": -34, "y": 8},
  "since_motion_ms": 0, "samples": 4127, "events": 12
}
```

`look` is already in the **−100..100** range the S3 node's
`/look?x=&y=` expects, so "turn the eyes toward whoever just walked in" is a
copy of two numbers with no coordinate maths in between.

The grid is a fixed size at every resolution, which is what lets
`/settings?framesize=` change resolution mid-flight without making the stored
baseline meaningless. Anything that legitimately changes the whole picture —
a resolution change, a flip, the flash coming on — re-baselines instead of
reporting a person.

It is a **change** detector, not a person detector: a curtain in a draught
moves it. Deciding whether the thing that moved is a face, and whose, is the
brain's job.

### `/settings` — fixing the mounting without a reflash

`framesize`, `quality`, `vflip`, `hmirror`, `brightness`, `contrast`,
`saturation`, `sharpness`, `awb`, `awb_gain`, `aec`, `aec2`, `ae_level`,
`aec_value`, `agc`, `agc_gain`, `gainceiling`, `bpc`, `wpc`, `raw_gma`,
`lenc`, `denoise`, `wb_mode`, `special_effect`, `colorbar`.

```
GET /settings                        # read everything back
GET /settings?vflip=1&hmirror=1      # camera ended up upside down on the head
GET /settings?framesize=xga          # more pixels across a distant face
GET /settings?ae_level=-1            # bright window behind the subject
```

Frame buffers are allocated for UXGA at boot even though the camera runs
smaller, because they are sized once from the initial framesize — so a later
request for a bigger frame has somewhere to put the picture.

## Hardware

- AI-Thinker ESP32-CAM (OV2640, 4 MB PSRAM)
- USB-to-serial adapter (FTDI / CP2102) for flashing — the board has no USB port
- **5V supply able to give ~500 mA.** See the power note above.

Different board? Set `CAMERA_MODEL_*` in `config.h` — `camera_pins.h` carries
AI-Thinker, ESP32-S3-EYE, XIAO ESP32S3 Sense, WROVER-KIT and ESP-EYE.

## Flashing

1. Arduino IDE → install the **esp32** boards package (Espressif Systems), 2.0.5 or newer.
2. **Tools → Board →** "AI Thinker ESP32-CAM"; **Partition Scheme →** "Huge APP"; **PSRAM →** "Enabled".
3. Copy `esp32-cam/robot_eye/config.example.h` to `config.h` and fill in your
   WiFi name and password. (`config.h` is gitignored, so your password stays local.)
4. Wire the adapter: `5V→5V`, `GND→GND`, `TX→U0R`, `RX→U0T`.
5. Hold **GPIO0 to GND**, press reset — the board is in flash mode. Upload.
6. Disconnect GPIO0, press reset. Serial Monitor at **115200 baud** prints the
   IP, the live-view URL, and the exact line to say to Iris to register it.

Open `http://<camera-ip>/` — you should see the robot's view, live, with the
status JSON updating underneath it.

## Config worth knowing about

All in `config.h`:

- `DEFAULT_FRAMESIZE` — `FRAMESIZE_SVGA` (800×600). Good balance: enough face
  detail at arm's length, ~40 KB a frame.
- `MOTION_CELL_DELTA` (18) — raise it if a flickering lamp triggers motion.
- `MOTION_MIN_PERCENT` (2) — how much of the grid must change. ~2% is a hand
  entering the frame, ~6% a person walking in.
- `ACCESS_TOKEN` — empty by default. Set it and every endpoint needs
  `?token=…` or an `X-Auth-Token` header. A lock on the LAN, not
  internet-facing security: keep the camera off the public internet.
- `STREAM_MAX_CLIENTS` (1) — a second viewer is refused rather than left to
  fight over the two frame buffers.

## Tests

```bash
esp32-cam/tests/run_tests.sh        # compiles the sketch + 136 motion checks
cd brain && python3 -m pytest -q    # 273 tests on the brain side
```

`run_tests.sh` does two things, neither needing an ESP32:

1. **Compiles the whole sketch** against small stubs for `Arduino.h`, WiFi,
   mDNS and the camera driver. It runs nothing — it just makes the compiler
   read all thousand lines of `robot_eye.ino`, which is otherwise only ever
   checked when you try to flash it and find out the hard way. It builds twice,
   the second time with `ENABLE_MOTION=0` and `ENABLE_FLASH_LED=0`, because an
   `#if` that only compiles one way round is not much of a switch. The config
   is generated from `config.example.h`, so this also proves the example config
   is a working one, and never touches your real `config.h`.
2. **Runs the motion tests** against the real `motion.h`, so what gets checked
   is the source that ships rather than a transliteration of it that could
   drift. They earned their keep: the centroid was being averaged into a whole
   cell index before it was scaled, throwing away up to half a cell — a
   standing bias up and to the left, so the eyes would have sat slightly off
   whoever they were meant to be facing.

The brain tests run on a bare Python install with no vision libraries and no
network, which is the same bar Iris sets for itself. The routing tests run
against a real Iris checkout (`IRIS_REPO=/path/to/iris_ai`) and check both that
every camera phrasing lands on the right tool and that nothing that worked
before now goes somewhere else.

## Troubleshooting

| Symptom | Look at |
|---|---|
| Reboots, or "camera init failed" | The 5V supply. `GET /status` → `reset_reason: "brownout"` says so outright. |
| Picture upside down or mirrored | `GET /settings?vflip=1&hmirror=1`. No reflash. |
| Face not recognised at conversational distance | `GET /settings?framesize=xga`. Pixels across the face is the biggest lever. |
| "There's someone there, but too far away" | Come closer, or raise the framesize. The brain is refusing to guess from a dozen pixels rather than answering wrongly. |
| Recognition is inconsistent | Introduce yourself two or three more times in different light. Each one is stored as a separate sample. |
| Stopped recognising you after installing a different recogniser | Embeddings are not portable between models. Say "forget every face you know" and re-introduce yourself; the store refuses to compare across backends rather than answering from noise. |
| `robot-eye.local` does not resolve | Use the IP. mDNS is re-announced after a WiFi reconnect, but some networks block it entirely. |
| Stream stutters | Drop to `FRAMESIZE_VGA`, or lower `STREAM_TARGET_FPS`. |
| "no PSRAM found" at boot | **Tools → PSRAM → Enabled**, and check the board really has it. Without it the camera is capped at VGA. |
