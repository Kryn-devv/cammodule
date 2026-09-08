# Wiring the eye into IRIS

Ten minutes of copying and five one-line edits. Every edit below quotes the
exact line to look for, because line numbers move.

This session could only push to `cammodule`, so the brain half lives here as a
drop-in kit rather than a commit on [Iris_AI](https://github.com/Kryn-devv/Iris_AI).
Nothing here is speculative — the routing and the logic are covered by the
tests in `brain/tests/`, which run against a real Iris checkout.

## 0. What goes where

| From here | To in Iris_AI |
|---|---|
| `brain/robot_eye/` | `iris/app/vision/robot_eye/` |
| `brain/iris_integration/camera.py` | `iris/app/tools/devices/camera.py` |

```bash
cd /path/to/Iris_AI
mkdir -p iris/app/vision
touch iris/app/vision/__init__.py
cp -r /path/to/cammodule/brain/robot_eye iris/app/vision/robot_eye
cp /path/to/cammodule/brain/iris_integration/camera.py iris/app/tools/devices/camera.py
```

`robot_eye` uses relative imports throughout, so it works at that path with no
edits. It imports nothing from Iris and nothing heavy at module scope — Iris
still boots and its tests still pass with no vision libraries installed.

## 1. Register the tool module

`iris/app/tools/loader.py` — find:

```python
    # Devices (ESP32 / home automation / robot)
    "iris.app.tools.devices.esp32",
    "iris.app.tools.devices.face",
```

add one line:

```python
    "iris.app.tools.devices.camera",
```

## 2. Allow "camera" as a device kind

`iris/app/tools/devices/registry.py` — find:

```python
DEVICE_KINDS = ("relay", "motor", "sensor", "face", "generic")
```

replace with:

```python
DEVICE_KINDS = ("relay", "motor", "sensor", "face", "camera", "generic")
```

Without this, `add device eye at ... as camera` is silently stored as
`generic` and `camera_look` will not find it.

## 3. Add the intent rules

`iris/app/nlu/rules.py`. Two edits.

**3a.** Paste the block from `brain/iris_integration/nlu_rules.py`
(`BLOCK_TO_PASTE`, or run `python3 brain/iris_integration/nlu_rules.py` to
print it) into the `RULES` list, **immediately after** the `device_status_query`
rule — the one ending:

```python
        confidence=0.9,
    ),
    Rule(
        name="hinglish_weather",
```

The position matters. Rules are first-match-wins, and `who_is` further down the
list sends "who is that" to Wikipedia. Put these after it and half of them
never fire.

**3b. Fix a normalizer bug** (worth doing regardless of the camera). Find:

```python
_TRAILING_POLITENESS = re.compile(r"\s*(?:please|for me|thanks|thank you|now)\s*[.!?]*$", re.IGNORECASE)
```

add `\b`:

```python
_TRAILING_POLITENESS = re.compile(r"\s*\b(?:please|for me|thanks|thank you|now)\s*[.!?]*$", re.IGNORECASE)
```

`now` has no word boundary, so it is stripped off the end of any word ending in
those letters: **"who do you know" normalizes to "who do you k"**, and "let it
snow" to "let it s". The `\b` fixes it and still strips real politeness
("turn on the light now" → "turn on the light"). Two camera phrasings depend on
this; `brain/tests/test_iris_rules.py::TestNormalizerFix` demonstrates both the
bug and the repair.

## 4. Configure a vision model

Object identification is the one part that needs a model. Face recognition does
not — that runs locally.

In `.env`:

```ini
# Any model that accepts images. Free options that work:
VISION_MODEL=meta-llama/llama-4-scout:free      # via OPENROUTER_API_KEY
# VISION_MODEL=gemini-flash-latest              # via GEMINI_API_KEY
```

`VISION_MODEL` already exists in `iris/app/core/config.py` and the gateway
already routes the `VISION` capability tag to it — it was just never set. Worth
adding to `.env.example` under the capability overrides:

```ini
# Vision — needed for "what is this?" / "read this label". Must accept images.
# VISION_MODEL=meta-llama/llama-4-scout:free
```

No gateway change is required. `CloudLLMProvider.generate` builds its request
as `_build_messages(prompt, system_prompt, kwargs["messages"])` and posts it to
an OpenAI-compatible `/chat/completions`, so the ready-made `messages` list
carrying the `image_url` block goes straight through.

If a slow free model times out, raise `PER_TOOL_TIMEOUT_SECONDS` (default 20).

## 5. Install a face recogniser

Faces are compared locally. Pick one:

```bash
pip install face-recognition          # dlib, 128-d. Needs a C++ toolchain.
pip install insightface onnxruntime   # ArcFace, 512-d. Downloads a model once.
```

Either also needs a JPEG decoder — `pip install pillow` (face-recognition
pulls it in anyway).

Installing neither is a supported state, not a broken one:

```bash
pip install opencv-python-headless    # finds faces, cannot name them
```

gets you "there's someone in front of me" and an honest note that naming them
needs a recogniser. With nothing at all installed, the face tools report what
to install and everything else keeps working.

**Embeddings are not portable between backends.** A face enrolled with dlib
cannot be compared to one from ArcFace — the numbers mean different things. The
store records which backend made each sample and refuses to cross that line,
with a message telling you to re-introduce yourself. So pick a backend before
enrolling, and if you switch later, say "forget every face you know" first.

## 6. Optional polish

**A bus topic for sightings.** `iris/app/core/bus.py`, next to the `NODE_*`
entries:

```python
    VISION_SIGHTING = "vision.sighting"
```

The tool already publishes there via `getattr`, falling back to
`SYSTEM_NOTICE`, so this is genuinely optional.

**Capability reporting.** `iris/app/core/platform_info.py`, in
`_CAPABILITY_SPECS`, so `/api/v1/system/capabilities` reports vision like it
reports everything else:

```python
    "face_recognition": (("face_recognition", "insightface"), (), "pip install face-recognition"),
    "face_detection": (("face_recognition", "insightface", "cv2"), (), "pip install opencv-python-headless"),
    "image_decode": (("PIL", "cv2"), (), "pip install pillow"),
```

The tools do their own backend selection with better messages, so this is for
the dashboard rather than for them.

## 7. Register the camera and introduce yourself

Flash the firmware (see the repo README), read the address off the serial
monitor, then say to Iris:

```
add device eye at 192.168.1.42 as camera
```

or `add device eye at robot-eye.local as camera` if mDNS resolves on your
network. The registry only accepts LAN addresses, which is what you want.

If you set `ACCESS_TOKEN` in the firmware's `config.h`, put it in the device's
notes as `token=<value>` — that is the one per-device free-text field the
registry already persists, so it needs no schema change.

Then, standing in front of the camera in decent light:

```
remember my face as Prakash
```

The first person introduced becomes the owner automatically, which is what
makes "who am I" answer **"Yes, that's you."** Say it two or three more times
in different light — a lamp on, a lamp off, near a window — and recognition
gets noticeably steadier. Each sample is stored, and matching compares against
all of them rather than one average.

Faces live in `<data_dir>/faces.json`, beside `devices.json`. It is readable
JSON on purpose: when the robot stops recognising you, you should be able to
open it and see why.

## 8. Check it

```
is anyone in front of you     -> camera_presence  (cheap, no frame transferred)
who am I                      -> camera_who       (local, no internet needed)
what is this                  -> camera_look       (needs VISION_MODEL)
who do you know               -> camera_known_faces
```

`GET /api/v1/tools` should now list the six `camera_*` tools.

## What was deliberately not taken

Three existing phrasings already had good answers and were left alone:

- **"is there any motion" / "is there anyone"** still go to the PIR sensor on
  the S3 node. The PIR watches the whole room; the camera only sees its own
  view. The camera answers the narrower "can you see anyone".
- **"look at me"** still turns the OLED eyes toward you via `face_emotion`.
- **"this is Prakash"** is not a rule — it would match "this is a test" just as
  happily. Said in conversation it still works: the LLM agent has
  `camera_remember_face` in its tool list and will ask for a name if it needs one.

`brain/tests/test_iris_rules.py` asserts all three stay that way.
