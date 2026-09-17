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
questions the teacher asked sit in the top bar (desktop only, hidden on
phones) and tick off the first time a scene answers each one; later scenes
add depth.

## Running it

**No build step, no install.** The folder is self-contained:

```
presentation/
├── index.html    the page (styles + engine)
├── content.js    every word on screen, labels, camera paths, speaker notes
├── vendor/       three.js r160 + GSAP 3.12 (local copies, so it works offline)
├── server.js     optional: a zero-dependency static server (node server.js)
└── README.md
```

- **Double-click `index.html`.** It works straight from disk or from an
  unzipped download. Libraries load from `vendor/` first and fall back to
  cdnjs.cloudflare.com if that folder is missing. Only the fonts need the
  internet, and they fall back to system faces without it.
- **Or serve it:** `node server.js` (port from `SERVER_PORT` or `PORT`,
  default 8080, binds all interfaces), then open `http://localhost:8080/`.
- If the 3D layer cannot start (no WebGL, blocked scripts), the page says why
  and continues as a readable scroll of the same content instead of hanging.

### Hosting on a Pterodactyl panel

Pterodactyl runs game servers in containers, so the site runs as a tiny Node
process on the port the panel gives it:

1. Create a server with the **Node.js** egg (any Node 18+ image). If your
   host has a "static website / nginx" egg, that works too — skip to step 3
   and point its document root at this folder.
2. In the panel's **File Manager** (or over SFTP), upload the contents of
   `presentation/` — `index.html`, `content.js`, `server.js` and the whole
   `vendor/` folder — into the server's root.
3. Set the **Startup Command** to:

   ```
   node server.js
   ```

   Pterodactyl injects `SERVER_PORT`, which `server.js` reads, and the
   server binds `0.0.0.0`, which the panel requires. If the egg asks for a
   main file, enter `server.js`; leave "install dependencies" empty (there
   are none).
4. Start the server. The console prints the URL; open
   `http://<node-ip>:<allocated-port>/` — the panel shows the IP and port
   on the server's Network tab. Put a domain in front of it with the
   panel's allocation or a reverse proxy if you want a clean link.

Any static host works as well (GitHub Pages, Netlify, a school web folder):
upload the same four items.

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
keyboard just lands each scene at its "everything visible" point. Space and
Enter on a focused button activate that button, not the deck.

The URL follows the scene (`#agent`, `#eye`, …), so you can deep-link straight
to one: `index.html#demo` or `index.html?scene=10`, and editing the hash on
an open page jumps there. `?projector=1` starts in projector mode;
`?q=low|mid|high` forces a quality tier.

## Editing the content

Everything a presenter might change lives in **`content.js`**, not in the
engine:

- `config` — the assistant's name (`NOVA`), the project it is built on
  (`IRIS`), the word the particles spell at the end, the team names and the
  class line for the credits.
- `scenes[]` — one object per scene: its heading, lede, bullet lines, chips,
  speaker `notes`, the 3D shape it uses, the camera path, how long it scrolls
  (`len`, in viewport heights), which side the copy sits on (`align`), any
  3D-anchored labels, connector lines, particle streams, the bead's path with
  the text it shows at each stage (`labelAt`), and phone tuning (`phoneLift`,
  `phoneZoom`).

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
| 4 | Agent? | plan → act → observe → repeat loop | **is it an agent? yes, when it needs to be** |
| 5 | Tools | six tool clusters | what it does — 86 tools |
| 6 | LLM | provider ring with failover | how it works — the multi-provider router |
| 7 | Memory | four crystals | how it works — memory types |
| 8 | Body | the robot | what it does — the three boards |
| 9 | The eye | ESP32-CAM board + frame frustum | how it works — this repo's endpoints |
| 10 | Example | an apple, an eye, a brain | all three — "is this apple ripe?" |
| 11 | Guardrails | three permission shells | how it works — safety |
| 12 | So… | the name in particles | the one-sentence answer + credits |

Facts come from the IRIS README and this repository's README and firmware.
Where something is planned rather than built (the brain's camera tool and
vision model are Phase 7 on the IRIS roadmap), the site says so on screen in
every scene that touches it: the agent loop's bead label and chip, the eye's
endpoint line, the worked example's eyebrow and chip, and the finale's
summary. The provider-failover animation is marked as an illustration and
colours no vendor as failing.

## Under the hood

- **Three.js** (r160, classic build from `vendor/`) renders ~9k–26k points
  with a custom shader. Every scene's shape is a `Float32Array` of positions
  and colours, generated once behind the loader; two shapes live on the GPU at
  a time and a single `uMix` uniform blends them, with a per-particle stagger
  and a mid-morph burst so the transition swirls instead of sliding. A second,
  larger and fainter draw of the same points stands in for bloom.
- **Scroll → progress**: each `<section>` is `len` viewport-heights tall with a
  sticky one-viewport view inside (small-viewport units, so a phone's address
  bar does not resize the scenes). A continuous value
  `g = sceneIndex + localProgress` drives the morph, the camera, the props and
  the bead. Morphs happen across scene boundaries; copy fades in and out
  inside each scene. Spin is scene-local: scenes with `spin: 0` always settle
  back to their authored orientation.
- **GSAP ScrollTrigger** reveals the copy panels (scrubbed); **ScrollToPlugin**
  animates keyboard jumps. Both are optional: without them the copy is simply
  always visible and jumps use native smooth scrolling.
- **Props**: HTML labels anchored to 3D points (projected every frame and
  hidden when they would land on the HUD, the toolbar or off-screen),
  `LineSegments` connectors, `Points` streams along a `CatmullRomCurve3`, and
  the bead with its trail. Props marked `fixedProps` do not spin with the hero
  object; the bead follows the object otherwise.
- **Quality**: tiers by cores / pointer type (`?q=` overrides); a governor
  steps resolution down and then the halo off if the frame rate drops below
  50 for two windows.
- **Accessibility**: honours `prefers-reduced-motion` (no ambient wobble, no
  spin, no autonomous streams, no intro morph, instant jumps), has a manual
  motion toggle, keyboard focus styles, a focus-trapped help dialog that
  returns focus on close, and a readable no-WebGL fallback.
- **Layout tiers**: 720p/768p projectors get a tighter type scale; short
  viewports (landscape phones) drop the lede and shrink the lists; phones put
  the copy on a plate and lift the 3D object above it.

Tested in headless Chromium at 1440×900, 1280×720, 390×844 and 844×390 with
zero console errors; the same scripts exercise keyboard navigation, deep
links, the tracker, projector mode, notes, `file://` loading, blocked-CDN and
fully offline loading, and the no-WebGL fallback.
