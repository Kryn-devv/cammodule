# NOVA — the presentation

An immersive, scroll-driven 3D website that explains the assistant to a class:
**how it works, whether it is an agent, and what it does.**

One GPU particle system is the star of the whole scroll. It assembles into a
holographic sphere, then morphs scene by scene into a voice waveform, the five
kernel layers, the agent loop, the tool constellation, the provider router,
memory crystals, the robot body, the ESP32-CAM eye, an apple (for the worked
example), the permission shield, and finally the assistant's name.

One sentence — *"Hey IRIS, is this apple ripe?"* — travels with you through
the scenes as an amber bead: it rides the waveform, falls through the five
layers, runs around the agent loop, bounces off a rate-limited provider,
leaves the camera as a JPEG and arrives at the vision model. The three
questions the teacher asked sit in the top bar and tick off as the scenes
answer them.

**Zero build step.** Open `presentation/index.html` in a browser. The 3D layer
loads Three.js and GSAP from a CDN, so it needs internet the first time.

## Presenting

| Key | Does |
|---|---|
| `↓` `→` `space` `PageDown` | Next scene |
| `↑` `←` `PageUp` | Previous scene |
| `1` … `9`, `0` | Jump to scene 1–10 |
| `Home` / `End` | First / last scene |
| `F` | Fullscreen |
| `C` | Projector mode: lifts the background, puts copy on plates, brightens the particles. Rehearse in the real room with this on |
| `N` | Speaker notes for the current scene (2–3 talking points each) |
| `M` | Toggle reduced motion |
| `R` | Restart from the top |
| `?` | Show the key list |

The dots on the right edge also jump between scenes. The bar along the bottom
is the overall progress. Scrolling with a mouse or trackpad works too; the
keyboard just lands each scene at its "everything visible" point.

The URL follows the scene (`#agent`, `#eye`, …), so you can deep-link straight
to one: `index.html#demo` or `index.html?scene=10`. `?projector=1` starts in
projector mode; `?q=low|mid|high` forces a quality tier.

## Editing the content

Everything a presenter might change lives in **`content.js`**, not in the
engine:

- `config` — the assistant's name (`NOVA`), the project it is built on
  (`IRIS`), the word the particles spell at the end, the team names and the
  class line for the credits.
- `scenes[]` — one object per scene: its heading, lede, bullet lines, chips,
  speaker `notes`, the 3D shape it uses, the camera path, how long it scrolls
  (`len`, in viewport heights), which side the copy sits on (`align`), any
  3D-anchored labels, connector lines, particle streams, and the bead's path
  with the text it shows at each stage (`labelAt`).

Copy is plain HTML strings, so `<b>`, `<code>` and the `hl` / `warm` / `cool`
highlight spans work inside headings and lines.

The palette is defined once as CSS tokens at the top of `index.html` and again
as the `P` object in `content.js` (the 3D shapes cannot read CSS variables).
Change both if you recolour.

## Scenes

| # | Scene | Shape | Answers |
|---|---|---|---|
| 1 | Meet | particle sphere | intro + the three questions |
| 2 | Listen | voice waveform | how it works — voice, languages, local-first |
| 3 | Kernel | five stacked layers | how it works — the 5-layer pipeline |
| 4 | Agent? | plan → act → observe → repeat loop | **is it an agent? yes** |
| 5 | Tools | six tool clusters | what it does — 86 tools |
| 6 | Brains | provider ring with failover | how it works — LLM router |
| 7 | Memory | four crystals | how it works — memory types |
| 8 | Body | the robot | what it does — ESP32 boards |
| 9 | The eye | ESP32-CAM board + frame frustum | how it works — this repo's endpoints |
| 10 | Example | an apple, an eye, a brain | all three — "is this apple ripe?" |
| 11 | Guardrails | three permission shells | how it works — safety |
| 12 | So… | the name in particles | the one-sentence answer + credits |

Facts come from the IRIS README and this repository's README and firmware.
Where something is planned rather than built (the brain's camera tool and
vision model are Phase 7 on the IRIS roadmap), the site says so on screen.

## Under the hood

- **Three.js** (r160, ES module via import map) renders ~9k–26k points with a
  custom shader. Every scene's shape is a `Float32Array` of positions and
  colours; two shapes live on the GPU at a time and a single `uMix` uniform
  blends them, with a per-particle stagger and a mid-morph burst so the
  transition swirls instead of sliding.
- **Scroll → progress**: each `<section>` is `len` viewport-heights tall with a
  sticky 100vh view inside. A continuous value `g = sceneIndex + localProgress`
  drives the morph, the camera, the props and the bead. Morphs happen across
  scene boundaries; copy fades in and out inside each scene. Because every
  frame derives its state from the scroll position, fast, reverse and keyboard
  scrolling can never desync.
- **GSAP ScrollTrigger** reveals the copy panels (scrubbed); **ScrollToPlugin**
  animates keyboard jumps.
- **Props**: HTML labels anchored to 3D points (projected every frame),
  `LineSegments` connectors, `Points` streams along a `CatmullRomCurve3`, and
  the bead with its trail. Props marked `fixedProps` do not spin with the hero
  object.
- **Bloom** via `UnrealBloomPass` on capable machines only. Quality auto-picks
  from cores / pointer type and steps down if the frame rate drops.
- **Accessibility**: honours `prefers-reduced-motion` (no ambient wobble, no
  spin, instant jumps), has a manual motion toggle, keyboard focus styles, and
  a readable no-WebGL fallback.

Tested in headless Chromium at 1440×900 and 390×844 with zero console errors;
keyboard navigation, deep links, the tracker, projector mode, notes and the
no-WebGL fallback are exercised by the same scripts.
