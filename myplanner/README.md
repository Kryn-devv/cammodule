# MyPlanner

A personal planner web app — to-do list + goals — that works at whatever scale you
want to plan at: a single day, the week, or the whole month.

**Zero dependencies, zero build step.** Open `index.html` in any browser and start
planning. Everything is saved in the browser's localStorage.

## Views

- **Day** — an hour-by-hour timeline (6 AM–11 PM) with a live "now" line, an
  "Anytime today" list for unscheduled tasks, and a daily progress bar.
- **Week** — Mon–Sun columns with drag-and-drop rescheduling, today highlighted,
  and a Weekly Goals side panel.
- **Month** — a calendar with task-count badges and category dots; click any date
  to jump into its Day view. Monthly Goals sit alongside.
- **Goals** — weekly and monthly goals with deadlines and progress. Link tasks to
  a goal and its progress updates automatically as you complete them; unlinked
  goals track progress manually.

## Features

- First-visit onboarding: pick Day / Week / Month, optionally seeded with examples
- Tasks with title, description, date, time slot (or "anytime"), priority
  (High / Medium / Low) and category (Work, Personal, Study, Health, Other)
- Search plus category/priority filters, applied across every view
- Dark mode (follows the system, with a manual toggle that persists)
- Rotating motivational quote and current date in the header
- Undo for deletions, backup & restore via copy-paste JSON
- Keyboard shortcuts: `N` new task, `1`–`4` switch views, `/` search
- Fully responsive down to phone widths

## Tech

Single-file vanilla HTML/CSS/JS (~2.4k lines). Fonts: Fraunces, Albert Sans and
Spline Sans Mono via Google Fonts (falls back to system faces offline). State is
stored under the `myplanner.v1` localStorage key.
