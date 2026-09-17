/* ============================================================
   NOVA presentation — CONTENT
   Everything a presenter might want to change lives here:
   names, team, copy, labels, camera paths, scene lengths.
   The engine in index.html reads window.NOVA_CONTENT.
   ============================================================ */
window.NOVA_CONTENT = (() => {
  const UTTERANCE = '“Hey IRIS, is this apple ripe?”';
  const config = {
    name: 'NOVA',                 // the assistant's name shown everywhere
    project: 'IRIS',              // the project it is built on (github.com/Kryn-devv/Iris_AI)
    sub: 'inside an AI agent',    // HUD subtitle
    finaleWord: 'NOVA',           // the word the particles spell at the end
    team: ['Your name', 'Teammate', 'Teammate'],
    className: 'Class · School · 2026',
  };

  // Colours used by the 3D shapes (keep in sync with the CSS tokens in index.html)
  const P = {
    teal: '#5EF2D0', amber: '#FFB35C', peri: '#8CC8FF', danger: '#FF5D7A', ok: '#7BE38B',
    pink: '#F58BD3', ink: '#EEF2FA', soft: '#A9B4CC', faint: '#6F7A96', pcb: '#1E6B5A',
    red: '#D8323F', red2: '#F25C5C', blush: '#E8C64A', stem: '#7A5230',
  };

  // ---- shared 3D anchor points (labels + shapes agree on these) ----
  const STACK_Y = [2.9, 1.45, 0, -1.45, -2.9];
  const STACK_R = 1.9;
  const ringPt = (r, deg, z = 0) => [Math.cos(deg * Math.PI / 180) * r, Math.sin(deg * Math.PI / 180) * r, z];
  const LOOP_R = 2.3;
  const LOOP = { plan: ringPt(LOOP_R, 90), act: ringPt(LOOP_R, 0), observe: ringPt(LOOP_R, -90), decide: ringPt(LOOP_R, 180) };
  const TOOLS = ['Desktop', 'Web', 'Content', 'Files', 'System', 'Automation'];
  const TOOL_COLORS = [P.teal, P.peri, P.amber, P.pink, P.danger, P.ok];
  const TOOL_POS = TOOLS.map((_, i) => ringPt(2.35, 90 + i * 60, (i % 2 ? -0.6 : 0.6)));
  const PROVIDERS = ['OpenRouter', 'Groq', 'Google AI Studio', 'Cerebras', 'Mistral', 'Together', 'GitHub Models', 'Hugging Face'];
  const PROV_POS = PROVIDERS.map((_, i) => ringPt(3.0, 90 - i * 45));
  const PROV_DOWN = 0;   // in the story, the 1st choice happens to be rate-limited (illustrative, no vendor is coloured as failing)
  const PROV_NEXT = 1;   // the 2nd choice answers
  const MEM = ['Working', 'Conversation', 'Long-term', 'Project'];
  const MEM_POS = MEM.map((_, i) => ringPt(1.9, 45 + i * 90));
  const MEM_COLORS = [P.teal, P.peri, P.amber, P.ok];

  // helper: n points around a circle (for streams)
  const circle = (r, n, z = 0, startDeg = 90, dir = -1) => Array.from({ length: n }, (_, i) => ringPt(r, startDeg + dir * i * (360 / n), z));

  const scenes = [
    {
      id: 'hero', label: 'Meet', len: 130, align: 'center', shape: 'sphere', spin: 0.15,
      cam: { pos: [0, 0, 7.6], look: [0, 0, 0], drift: [0, 0, -0.8] },
      html: `
        <p class="kicker">${config.project} · a class presentation</p>
        <h1>${config.name}</h1>
        <p class="lede">An AI assistant that lives on your laptop, listens to your voice, and drives a robot body.</p>
        <div class="qs"><span><i>01</i>How does it work?</span><span><i>02</i>Is it an agent?</span><span><i>03</i>What does it do?</span></div>`,
      notes: [
        'Open with the three questions the teacher asked — the tracker at the top ticks them off as we go.',
        `${config.name} is our name for the brain; it is built on the ${config.project} project (github.com/Kryn-devv/Iris_AI).`,
        'One sentence — “Hey IRIS, is this apple ripe?” — will travel with us through every scene as the amber bead.',
      ],
    },
    {
      id: 'what', label: 'Listen', len: 120, align: 'left', shape: 'wave', spin: 0,
      q: 'How does it work?', eyebrow: 'What is it',
      cam: { pos: [0, 1.2, 8.6], look: [0, 0, 0], drift: [0, -0.8, -1.0] },
      heading: 'You talk. <span class="hl">It listens.</span>',
      lede: `${config.project} is a desktop assistant that lives on your machine. Say the wake word, then speak.`,
      lines: [
        `<span class="warm">${UTTERANCE}</span>`,
        'Speech → text in the browser (Chrome / Edge), or fully offline',
        'English, Hindi and Hinglish, language auto-detected',
        'Zero API keys needed for basic commands',
        'Local-first: your files and memory stay on your machine',
        'Speech can run fully offline with faster-whisper',
      ],
      chips: ['offline: <b>faster-whisper</b> listens · <b>piper</b> speaks', 'voices: <b>Zira / Aria</b> · <b>Samantha</b> · <b>Swara</b>'],
      bead: { path: [[-4.6, 0.2, 0.5], [-2.4, 0.9, 0.5], [0, -0.2, 0.5], [2.4, 0.8, 0.5], [4.6, 0.1, 0.5]], label: 'hey iris, is this apple ripe?' },
      notes: [
        'Voice is the interface: wake word first, then speech-to-text — in the browser or fully offline with faster-whisper.',
        'Three languages with auto-detection: English, Hindi, Hinglish.',
        'Free-first and local-first: no API key is needed for basic commands; files and memory stay on the machine. In-browser speech recognition (Web Speech API) uses the browser vendor’s service; faster-whisper keeps it fully offline.',
      ],
    },
    {
      id: 'pipeline', label: 'Kernel', len: 190, align: 'right', shape: 'stack', spin: 0, fixedProps: true,
      q: 'How does it work?', eyebrow: 'The kernel',
      cam: { pos: [0, 8.2, 9.6], look: [0, 0, 0], drift: [0, -8.6, 0] }, shift: -1.6,
      heading: 'Every sentence falls through <span class="hl">five layers</span>',
      lede: 'The first layer that can answer, does. Cheap first, the LLM last.',
      lines: [
        '1 · Wake word — did you say “hey iris”?',
        '2 · Memory — remember, recall, forget',
        '3 · Small talk — offline replies',
        '4 · Deterministic NLU — 80 intent rules, tool dispatch in milliseconds, offline',
        '5 · LLM agent loop — plan, call tools, observe, repeat',
      ],
      labels: [
        { text: '1 · Wake word', pos: [-(STACK_R - 0.35), STACK_Y[0] + 0.12, 0.9], c: P.teal },
        { text: '2 · Memory commands', pos: [-(STACK_R - 0.35), STACK_Y[1] + 0.12, 0.9], c: P.peri },
        { text: '3 · Small talk', pos: [-(STACK_R - 0.35), STACK_Y[2] + 0.12, 0.9], c: P.pink },
        { text: '4 · NLU · 80 rules · ms', pos: [-(STACK_R - 0.35), STACK_Y[3] + 0.12, 0.9], c: P.amber },
        { text: '5 · LLM agent loop', pos: [-(STACK_R - 0.35), STACK_Y[4] + 0.12, 0.9], c: P.ok },
      ],
      bead: { path: [[0, STACK_Y[0] + 1.6, 0.2], [0, STACK_Y[0], 0.2], [0, STACK_Y[2], 0.2], [0, STACK_Y[4], 0.2], [0, STACK_Y[4] - 1.0, 0.2]],
        labelAt: [[0.2, '1 · wake word ✓ “hey iris”'], [0.4, '2 · a memory command? no'], [0.6, '3 · small talk? no'], [0.8, '4 · NLU: 0 of 80 rules match'], [1.1, '5 · hand it to the agent']] },
      notes: [
        'Every sentence falls through the five kernel layers; the first that can handle it, does.',
        'Layers 1–4 need no LLM provider: wake word, memory commands, offline small talk, and 80 deterministic NLU intent rules that dispatch tools in milliseconds, offline.',
        'Only when none match (“is this apple ripe?” matches no rule) does layer 5, the LLM agent loop, wake up. An event bus streams every step to the UI over WebSocket.',
      ],
    },
    {
      id: 'agent', label: 'Agent?', len: 170, align: 'left', shape: 'loop', spin: 0, phoneLift: 2.4,
      q: 'Is it an agent?', eyebrow: 'The big question',
      cam: { pos: [1.0, 0.3, 9.6], look: [0, 0, 0], drift: [-2.0, 0.2, 0] }, shift: 2.1,
      heading: 'Is it an agent? <span class="hl">Yes</span> — when it needs to be.',
      lede: 'A chatbot answers once. An agent decides, acts through tools, checks the result, and repeats until done.',
      lines: [
        '<b>Plan</b> — the LLM decides what to do next and which tool to call',
        '<b>Act</b> — it calls the tool (function calling)',
        '<b>Observe</b> — it reads what came back',
        '<b>Repeat</b> — until the task is finished, then it answers you',
        '<span class="soft">“Is this apple ripe?” matches none of the 80 rules, so it lands here.</span>',
      ],
      chips: ['<b>86 tools</b> exposed as functions', 'layer 5 of the kernel', '<span class="soft">the loop is built · the camera tool it calls here is Phase 7, planned</span>'],
      labels: [
        { text: 'Plan', pos: [LOOP.plan[0], LOOP.plan[1] + 0.62, 0], c: P.peri, cls: 'big' },
        { text: 'Act · tool call', pos: [LOOP.act[0] + 0.1, LOOP.act[1] + 0.8, 0], c: P.teal, cls: 'big' },
        { text: 'Observe', pos: [LOOP.observe[0], LOOP.observe[1] - 0.62, 0], c: P.amber, cls: 'big' },
        { text: 'Done? else repeat', pos: [LOOP.decide[0] + 0.1, LOOP.decide[1] + 0.8, 0], c: P.ok, cls: 'big' },
        { text: 'LLM', pos: [0, 0, 0.7], c: P.peri, cls: 'plain big' },
      ],
      streams: [{ path: circle(LOOP_R, 32), closed: true, count: 40, color: P.teal, speed: 0.12, size: 0.16 }],
      bead: { path: circle(LOOP_R, 16, 0.25, 90, -1).concat([ringPt(LOOP_R, 90, 0.25)]),
        labelAt: [[0.25, 'plan: “I need to look at it”'], [0.5, 'act: call the camera tool (planned)'], [0.75, 'observe: one JPEG came back'], [1.1, 'done? → yes, answer']] },
      notes: [
        'This is the definition the teacher is asking about: a chatbot replies once; an agent plans, acts with tools, observes the result and repeats until done.',
        'Layer 5 of the kernel is this loop: the IRIS README calls it an “LLM agent loop with function calling across all tools”. Plan → act → observe → repeat is our reading of that loop.',
        'Honest nuance: “open YouTube” hits one NLU rule and runs in milliseconds without the loop. The agent runs only when no cheaper layer can answer — so: an agent, when it needs to be.',
        'The camera tool in this example is Phase 7 (planned); the loop itself and its 86 tools are built.',
      ],
    },
    {
      id: 'tools', label: 'Tools', len: 160, align: 'right', shape: 'constellation', spin: 0.12,
      q: 'What does it do?', eyebrow: 'What it does',
      cam: { pos: [0, 0.3, 10.2], look: [0, 0, 0], drift: [0, -0.3, -1.0] }, shift: -2.3,
      heading: '<span class="hl">86&nbsp;tools</span>, six&nbsp;families',
      lede: 'Everything the agent can reach for, from opening an app to writing a deck.',
      lines: [
        '<b>Desktop</b> — apps, windows, volume, screenshots',
        '<b>Web</b> — weather, news, YouTube, Maps, Amazon search',
        '<b>Content</b> — PowerPoint decks, Word docs, spreadsheets, code projects',
        '<b>Files</b> — your documents and folders',
        '<b>System</b> — clipboard, keyboard & mouse, power (guarded)',
        '<b>Automation</b> — reminders, routines, timers',
      ],
      labels: TOOLS.map((t, i) => ({ text: t, pos: [TOOL_POS[i][0] * 1.32, TOOL_POS[i][1] * 1.32, TOOL_POS[i][2]], c: TOOL_COLORS[i] })),
      links: TOOL_POS.map(p => ({ a: [0, 0, 0], b: p, c: P.faint })),
      notes: [
        '“What does it do?” — 86 tools in six families, all reachable by voice.',
        'Desktop and system control, web lookups, creating decks / documents / spreadsheets / code, files, and automation (reminders, routines, timers).',
        'Power commands are guarded — that is the guardrails scene later.',
      ],
    },
    {
      id: 'brain', label: 'LLM', len: 150, align: 'left', shape: 'providers', spin: 0.05, phoneLift: 2.6,
      q: 'How does it work?', eyebrow: 'The LLM',
      cam: { pos: [0, 0.5, 11.4], look: [0, 0.15, 0], drift: [0, -0.5, -0.4] }, shift: 2.0,
      heading: 'One brain, <span class="cool">eight model providers</span>',
      lede: `${config.project} does not depend on a single model provider. The failover below is an illustration, not a report.`,
      lines: [
        'Tries your providers in your preferred order',
        'If one rate-limits, it fails over automatically',
        'That provider gets a cool-down (a “circuit breaker”), then is tried again',
        'No provider at all? Local commands still work offline',
      ],
      chips: PROVIDERS.map((p, i) => `${i + 1} · ${p}`),
      labels: PROVIDERS.map((p, i) => { const k = (i === 6) ? 0.66 : 1.22; return { text: `${i + 1} · ${p}`, pos: [PROV_POS[i][0] * k, PROV_POS[i][1] * k, 0], c: P.peri }; })
        .concat([{ text: 'router', pos: [0, -0.75, 0.4], c: P.peri, cls: 'plain' }]),
      links: PROV_POS.map(p => ({ a: [0, 0, 0], b: p, c: P.faint })),
      bead: { path: [[0, 0, 0.3], PROV_POS[PROV_DOWN].map((v, k) => k === 2 ? 0.3 : v * 0.9), [0.2, -0.2, 0.3], PROV_POS[PROV_NEXT].map((v, k) => k === 2 ? 0.3 : v * 0.9)],
        labelAt: [[0.35, 'router → 1st choice'], [0.62, 'rate-limited (example) → cool-down, try the next'], [1.1, '2nd choice answers ✓']] },
      notes: [
        'The LLM is not one vendor: the router tries the configured providers in the user’s preferred order.',
        'If one rate-limits, it is put on cool-down (the README calls this circuit-breaker cooling) and the next provider answers — automatic failover. Which vendor fails is an illustration here, not a report.',
        'With no provider at all, layers 1–4 still answer without any LLM. Basic commands need zero API keys.',
      ],
    },
    {
      id: 'memory', label: 'Memory', len: 120, align: 'right', shape: 'crystals', spin: 0.2,
      q: 'How does it work?', eyebrow: 'Memory',
      cam: { pos: [0, 1.0, 8.8], look: [0, -0.2, 0], drift: [0, -0.9, -0.4] },
      heading: 'It <span class="hl">remembers</span>',
      lede: 'Four kinds of memory, persisted in SQLite on your machine.',
      lines: [
        '<b>Working</b> — what we are doing right now',
        '<b>Conversation</b> — what was just said',
        '<b>Long-term</b> — “remember that my exam is on Friday”',
        '<b>Project</b> — files and context for the thing you are building',
      ],
      labels: MEM.map((m, i) => ({ text: m, pos: [MEM_POS[i][0] * 1.45, MEM_POS[i][1] * 1.45 + 0.2, 0], c: MEM_COLORS[i] })),
      notes: [
        'Four kinds of memory — working, conversation, long-term and project — persisted in SQLite on the machine.',
        '“Remember / recall / forget” are handled directly by kernel layer 2, without the LLM.',
      ],
    },
    {
      id: 'body', label: 'Body', len: 160, align: 'left', shape: 'robot', spin: 0, turn: Math.PI * 0.55,
      q: 'What does it do?', eyebrow: 'The body',
      cam: { pos: [3.6, 1.4, 7.4], look: [0, 0.1, 0], drift: [-3.4, -0.6, 0.4] }, shift: 1.5,
      heading: 'It has a <span class="warm">body</span>',
      lede: 'Three boards on WiFi turn the assistant into a robot.',
      lines: [
        '<b>Motor driver board</b> (BTS7960) — moves the robot',
        '<b>Sensor board</b> (ESP32-S3) — OLED eyes, ultrasonic, temperature, motion, gas, flame, light',
        '<b>ESP32-CAM</b> — the eye. Its firmware is this repo, <code>cammodule</code>',
        '<span class="soft">Commands travel as UDP datagrams — milliseconds</span>',
      ],
      labels: [
        { text: 'OLED eyes', pos: [0.95, 1.45, 0.5], c: P.teal },
        { text: 'ESP32-CAM · the eye', pos: [1.1, 0.55, 0.7], c: P.amber },
        { text: 'ultrasonic', pos: [-1.15, -0.75, 0.75], c: P.peri },
        { text: 'BTS7960 motor driver', pos: [1.75, -1.25, 0.3], c: P.soft },
        { text: 'WiFi · UDP', pos: [0.35, 2.3, 0], c: P.amber },
      ],
      notes: [
        'The assistant has a body: three WiFi microcontroller boards. Commands are UDP datagrams, so latency is milliseconds.',
        'The motor driver board (BTS7960) moves the robot; the ESP32-S3 sensor board has OLED eyes, ultrasonic, DHT22 temperature/humidity, PIR motion, gas, flame and light sensors.',
        'The ESP32-CAM is the eye — its firmware is this repository, cammodule.',
      ],
    },
    {
      id: 'eye', label: 'The eye', len: 160, align: 'right', shape: 'eye', spin: 0, phoneLift: 2.4,
      q: 'How does it work?', eyebrow: 'cammodule',
      cam: { pos: [2.4, 1.2, 8.4], look: [0, 0.5, 1.0], drift: [-2.4, -0.6, 0] },
      heading: 'The <span class="warm">eye</span> is a tiny web server',
      lede: 'An AI-Thinker ESP32-CAM sits on the WiFi and answers HTTP. It knows nothing about the brain.',
      lines: [
        '<code>GET /capture</code> — one fresh JPEG. This is what the brain will call',
        '<code>GET :81/stream</code> — live MJPEG on its own port, so viewers never block the brain',
        '<code>GET /status</code> — health as JSON: resolution, WiFi signal, uptime',
        '<code>GET /flash?on=1</code> — light the scene when it is dark',
      ],
      chips: ['<b>robot-eye.local</b> via mDNS', '<b>800×600</b> · ~40 KB per frame', '<b>OV2640</b> sensor'],
      labels: [
        { text: 'OV2640 sensor', pos: [0.95, 0.35, 0.4], c: P.amber },
        { text: 'WiFi antenna', pos: [0.75, 2.0, 0.1], c: P.teal },
        { text: 'flash LED', pos: [-1.05, 1.5, 0.1], c: P.ink },
      ],
      links: [
        { a: [0, 0.6, 0.4], b: [-1.25, 1.3, 2.9], c: P.amber }, { a: [0, 0.6, 0.4], b: [0.65, 1.3, 2.9], c: P.amber },
        { a: [0, 0.6, 0.4], b: [-1.25, -0.1, 2.9], c: P.amber }, { a: [0, 0.6, 0.4], b: [0.65, -0.1, 2.9], c: P.amber },
        { a: [-1.25, 1.3, 2.9], b: [0.65, 1.3, 2.9], c: P.ink }, { a: [0.65, 1.3, 2.9], b: [0.65, -0.1, 2.9], c: P.ink },
        { a: [0.65, -0.1, 2.9], b: [-1.25, -0.1, 2.9], c: P.ink }, { a: [-1.25, -0.1, 2.9], b: [-1.25, 1.3, 2.9], c: P.ink },
      ],
      bead: { path: [[0, 0.6, 0.5], [-0.3, 0.6, 2.9], [-0.7, 0.75, 6.8]], labelAt: [[0.45, 'GET /capture'], [1.1, 'one JPEG · 800×600 · ≈40 KB']] },
      notes: [
        'The eye is an AI-Thinker ESP32-CAM (OV2640 sensor) running our robot_eye.ino firmware — a tiny HTTP server on the WiFi.',
        'GET /capture returns one fresh JPEG — that is the call the brain will make once its camera tool lands (Phase 7). The MJPEG stream lives on port 81 so a viewer can never block the brain.',
        '/status gives JSON health, /flash?on=1 lights the scene, and mDNS makes it reachable as robot-eye.local. Pull-style: the eye knows nothing about the brain.',
      ],
    },
    {
      id: 'demo', label: 'Example', len: 230, align: 'left', shape: 'apple', spin: 0.25, shift: 1.6, fixedProps: true, phoneLift: 2.6,
      q: 'All three questions', eyebrow: 'Worked example · Phase 7, planned',
      cam: { pos: [0, 0.7, 9.0], look: [0, 0.5, 0], drift: [0, -0.2, -1.0] },
      heading: '“Hey IRIS, <span class="hl">is this apple ripe?</span>”',
      steps: [
        'Wake word heard: “hey iris” <small>layer 1 · it starts listening to you</small>',
        'No memory command, no small talk, no rule knows “ripe” <small>layers 2–4 pass</small>',
        'Agent loop: <b>plan</b> → call the camera tool <small>layer 5 · the LLM picks a tool</small>',
        '<code>GET http://robot-eye.local/capture</code> → one JPEG <small>act · the tool runs · the eye answers in one request</small>',
        'Frame → base64 → vision model: “list the objects; judge whether any fruit looks ripe” <small>act · inside the tool</small>',
        '<b>Observe</b> the answer, decide the task is done <small>the loop stops</small>',
        `${config.name} speaks: “The apple looks ripe — deep red skin, no green near the stem.”`,
      ],
      chips: ['<b>vision</b> is Phase 7 on the roadmap: the eye is ready, the camera tool is next'],
      labels: [
        { text: 'robot-eye.local', pos: [1.7, 2.1, 1.0], c: P.amber },
        { text: 'brain · vision model', pos: [-1.3, 2.7, 0], c: P.peri },
      ],
      links: [
        { a: [1.7, 1.8, 1.0], b: [0.9, 1.4, 0.6], c: P.amber }, { a: [1.7, 1.8, 1.0], b: [0.9, -1.0, 0.6], c: P.amber },
        { a: [1.7, 1.8, 1.0], b: [-0.7, 1.4, 1.3], c: P.amber }, { a: [1.7, 1.8, 1.0], b: [-0.7, -1.0, 1.3], c: P.amber },
      ],
      bead: { path: [[1.7, 1.8, 1.0], [1.5, 2.2, 1.0], [0.3, 3.0, 0.6], [-1.3, 2.7, 0]], labelAt: [[0.5, 'JPEG → base64 → vision model'], [1.1, '“the apple looks ripe”']] },
      notes: [
        'Trace one sentence end to end: wake word → layers 2–4 pass → the agent loop plans → GET /capture on the eye → vision model → spoken answer.',
        'Be honest: the eye is built and serving frames today; the brain’s camera tool and vision model are Phase 7 on the roadmap.',
        'This one example answers all three questions at once: how it works, why it is an agent, and what it can do.',
      ],
    },
    {
      id: 'safety', label: 'Guardrails', len: 150, align: 'left', shape: 'shield', spin: 0.1, fixedProps: true, phoneLift: 2.4,
      q: 'How does it work?', eyebrow: 'Guardrails',
      cam: { pos: [0, 1.6, 9.2], look: [0, 0.3, 0], drift: [0, -0.8, -0.6] }, shift: 1.6,
      heading: 'Powerful, so it is <span class="hl">guarded</span>',
      lede: `Every tool carries a risk grade. The riskier the action, the more ${config.project} asks first.`,
      lines: [
        '<b>READ</b> — just look. Safe, automatic',
        '<b>CONFIRM_REQUIRED</b> — asks you before acting',
        '<b>HIGH_RISK_ACTION</b> — the dangerous ones, extra guarded',
        'Sandbox: never touches <code>~/.ssh</code>, key files or <code>.env</code>',
        'Denylist blocks <code>rm -rf /</code> and fork bombs',
        'Remote access needs a bearer token and is rate-limited',
      ],
      chips: ['<span class="ok">“what’s the weather?”</span> → a READ tool · runs', '<span class="bad">“shut down the PC”</span> → guarded · you confirm, or nothing happens'],
      notes: [
        'Powerful tools need locks: every tool carries a risk grade — READ, CONFIRM_REQUIRED, HIGH_RISK_ACTION.',
        'A filesystem sandbox blocks ~/.ssh, key files and .env; a denylist refuses destructive commands like rm -rf / and fork bombs.',
        'Remote access (phone pairing by QR) needs a bearer token and is rate-limited; there is also a Telegram bot. Local-first by default.',
      ],
      labels: [
        { text: 'READ', pos: [1.25, 1.15, 0], c: P.ok },
        { text: 'CONFIRM_REQUIRED', pos: [1.85, 1.65, 0], c: P.amber },
        { text: 'HIGH_RISK_ACTION', pos: [2.45, 2.15, 0], c: P.danger },
        { text: 'your machine', pos: [0, -0.05, 0.6], c: P.teal, cls: 'plain' },
      ],
    },
    {
      id: 'finale', label: 'So…', len: 140, align: 'bottom', shape: 'text', spin: 0, phoneLift: 4.4, phoneZoom: 2.6,
      cam: { pos: [0, -2.3, 9.4], look: [0, -2.3, 0], drift: [0, 0, -0.6] },
      html: `
        <p class="eyebrow"><span class="q">So…</span> is it an agent?</p>
        <h2><span class="hl">Yes.</span> Simple asks take a rule. The rest runs plan → act → observe until done.</h2>
        <div class="chips">
          <span><b>Perceives</b> · voice, sensors — camera next (Phase 7)</span>
          <span><b>Decides</b> · five layers, then an LLM that plans</span>
          <span><b>Acts</b> · 86 tools + a robot body</span>
          <span><b>Remembers</b> · four memories in SQLite</span>
          <span><b>Asks first</b> · when an action is risky</span>
        </div>
        <p class="credits"><b>${config.team.join(' · ')}</b><br>${config.className} · built on ${config.project} (github.com/Kryn-devv/Iris_AI) · eye firmware: cammodule</p>
        <button class="again" id="btnAgain">Play it again</button>`,
      notes: [
        'The verdict in one sentence: yes, it is an agent when it needs to be — simple commands take a rule; everything else runs plan → act → observe until done.',
        'Recap the three questions with the tracker fully ticked; then credits and questions from the class.',
      ],
    },
  ];

  const anchors = { STACK_Y, STACK_R, LOOP, LOOP_R, TOOL_POS, TOOL_COLORS, PROV_POS, PROV_DOWN, PROV_NEXT, MEM_POS, MEM_COLORS };
  return { config, palette: P, anchors, scenes };
})();
