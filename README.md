# cammodule — the robot's eye (ESP32-CAM)

Firmware that turns an AI-Thinker **ESP32-CAM** into a small camera server on
your WiFi network. The main brain ([NOVA / Iris_AI](https://github.com/Kryn-devv/Iris_AI))
pulls frames from it whenever it wants to look at the world — e.g. to check
whether the apple in front of the robot looks ripe.

```
+-------------+        GET /capture (JPEG)        +----------------+
|  ESP32-CAM  | <-------------------------------- |   NOVA brain   |
|  robot_eye  | --------------------------------> | (vision model) |
+-------------+        one fresh frame            +----------------+
      |
      | :81/stream (MJPEG)
      v
  your browser (live view)
```

The camera never needs to know anything about the brain. It just sits on the
network and answers:

| Endpoint | What it does |
|---|---|
| `GET /` | Info page with a live preview |
| `GET /capture` | **One fresh JPEG frame — this is what the brain calls** |
| `GET /status` | JSON health: camera id, resolution, WiFi RSSI, heap, uptime |
| `GET /flash?on=1` | Toggle the white flash LED (`on=0` to turn off) |
| `GET :81/stream` | MJPEG live stream (port 81, for watching in a browser) |

The stream runs on its own port so a browser watching the live feed can never
block the brain from grabbing a `/capture` frame.

With mDNS the camera is reachable as `http://robot-eye.local` — no hardcoded
IP needed.

## Hardware

- AI-Thinker ESP32-CAM (OV2640 sensor)
- USB-to-serial adapter (FTDI / CP2102) for flashing — the board has no USB port
- 5V supply that can deliver ~500 mA (brownouts are the #1 cause of weird resets)

Different ESP32 camera board? Swap the pin map in
`esp32-cam/robot_eye/camera_pins.h`.

## Flashing

1. In the Arduino IDE, install the **esp32** boards package (Espressif Systems).
2. Select **Tools > Board > AI Thinker ESP32-CAM**, Partition Scheme **Huge APP**.
3. Copy `esp32-cam/robot_eye/config.example.h` to `config.h` and fill in your
   WiFi name/password (config.h is gitignored, so your password stays local).
4. Wire the serial adapter: `5V->5V`, `GND->GND`, `TX->U0R`, `RX->U0T`.
5. Hold **GPIO0 to GND**, press reset — the board is now in flash mode. Upload.
6. Disconnect GPIO0, press reset. Open the Serial Monitor at **115200 baud**:
   the camera prints its IP and the live-view URL.

Open `http://<camera-ip>/` in a browser — you should see the robot's view live.

## Connecting it to the NOVA brain

NOVA doesn't have a vision endpoint yet (`VISION_MODEL` is `None`; vision is
Phase 7 on its roadmap), so the camera is built pull-style: when you add
vision to NOVA, its camera tool just fetches a frame and hands it to the
vision model. The whole integration is one HTTP GET:

```python
import httpx

CAMERA_URL = "http://robot-eye.local/capture"  # or http://<camera-ip>/capture

async def grab_frame() -> bytes:
    """One fresh JPEG from the robot's eye."""
    async with httpx.AsyncClient(timeout=5.0) as client:
        r = await client.get(CAMERA_URL)
        r.raise_for_status()
        return r.content  # JPEG bytes -> base64 it into your vision model call
```

Sketch of the NOVA-side tool (a `BaseTool` in `nova/app/tools/builtin/`):
grab the frame, base64-encode it, send it to `VISION_MODEL` with a prompt like
*"List the objects you see. For any fruit or produce, judge whether it looks
ripe and explain the visual cues."* — and NOVA can answer things like
"the grape and the apple both look ripe" through its normal chat flow.

`GET /status` is handy for a NOVA health check before grabbing frames, and
`GET /flash?on=1` lets the brain light up the scene when it's too dark.

## Tuning

All in `config.h` / `robot_eye.ino`:

- **Resolution**: `FRAMESIZE_SVGA` (800x600) by default — a good balance of
  detail for ripeness judgment vs. frame size. Bump to `FRAMESIZE_XGA`/`UXGA`
  for more detail, drop to `FRAMESIZE_VGA` if the stream stutters.
- **JPEG quality**: `jpeg_quality = 12` (lower number = better quality).
- **Multiple eyes**: give each board its own `CAMERA_ID` and `MDNS_NAME`.
