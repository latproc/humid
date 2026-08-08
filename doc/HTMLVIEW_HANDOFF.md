# HTMLVIEW / operators manual viewer — handoff

**Date:** 2026-08-08  
**Purpose:** Resume work in a fresh session without relying on compacted chat context.

---

## Goal

Add an **in-process HTML document viewer** to Humid for operator manuals (e.g. 4C04):

- Content served **remotely** (URL from Clockwork, not a local file tree as primary source)
- **Embedded** in Humid (no external browser — panels have **no usable window manager**)
- Multi-resolution panels (4C04 **1280×800**; others often **1920×1080 / 1920×1200**)
- Screen selection + **Return** like IOCORE (`M_CoreControl2Panel.Reset`)
- **Open source only** — Ultralight rejected (commercial)

Manual source of truth lives outside humid:

`/Users/mike/src/CW_Simulation/4C04/Docs/operators-manual/`  
(MD → pandoc HTML + `manual.css` + `images/**` + optional PDF)

---

## Branches

| Branch | Status |
| --- | --- |
| **`feature/html-document-view`** | Active feature work. Working tree has HTMLVIEW + bake-off + demo (**much uncommitted**). Base was `cw-no-ec-tools-compatiblity` @ `9fcea0d`. |
| **`cw-no-ec-tools-compatiblity`** | Production panel line. **Local commit ahead 1** (not pushed): headless `hmifile_check`. |

### Production commit (local only, not pushed)

```
7a1fa6d Make hmifile_check headless (no NanoGUI/GLFW).
```

Worktree used for that commit (can remove if leftover):

`/tmp/humid-prod-hmicheck`

### Feature branch uncommitted work (high level)

```
M  CMakeLists.txt, .gitignore
M  src/structure.cpp, widgetfactory.*, userwindow, dialogwindow, factorybuttons, linkableobject
M  src/curl_helper.cpp (timeouts, null-handle cleanup)
M  src/anchor.cpp (+ new anchor_widget.cpp already on prod commit)
?? cmake/Modules/HumidLitehtml.cmake
?? src/editorhtmlview.{h,cpp}
?? src/htmlview_container.{h,cpp}
?? lib/litehtml/          # vendored (~4.7M, BSD-3)
?? demo/htmlview/
?? doc/html-viewer-bakeoff.md
?? doc/HTMLVIEW_HANDOFF.md (this file)
?? tools/html-viewer-bakeoff/  # mostly gitignored clones/outputs
?? panels.txt
```

**Recommend:** commit feature branch work soon so nothing is lost; keep HTMLVIEW separate from the small prod `hmifile_check` commit.

---

## Architecture (decided)

```
CW: V_CoreControl2HMI = "Manual"
    V_ManualURL = "http://host:port/.../Manual.html"   # planned product wiring
        │
        ▼
Humid MANUAL.humid
  HTMLVIEW(remote → URL string, or static url: property)
    curl fetch HTML + CSS + images (disk cache)
    litehtml layout @ widget width
    Cairo/Pango paint viewport → NVG texture
    scroll + anchors
  Return → M_CoreControl2Panel.Reset  (product; demo button is chrome-only)
```

### Engine choice

| Engine | Result |
| --- | --- |
| **litehtml** (BSD-3) | **Ship path.** Renders 4C04 manual (tables, TOC, PNG images). |
| **poxk/Falco** (MIT) | Built, but produced **all-white** PNGs — not usable. |
| **Ultralight** | Rejected (not open source / commercial). |
| External browser | Rejected (no WM on panels). |

Bake-off notes: `doc/html-viewer-bakeoff.md`  
Artifacts (local): `tools/html-viewer-bakeoff/out/` (gitignored)

### Dependencies for HTMLVIEW

- **CMake option:** `HUMID_WITH_HTMLVIEW` (default ON if Cairo/Pango found)
- **Module:** `cmake/Modules/HumidLitehtml.cmake`
- **pkg-config modules:** `cairo`, `pangocairo`, `fontconfig` (plus libcurl, already required by humid)
- **Install (panels / Debian / RPi OS):**
  ```bash
  sudo apt-get install -y libcairo2-dev libpango1.0-dev libfontconfig1-dev pkg-config
  ```
- **Install (macOS Homebrew):**
  ```bash
  brew install cairo pango fontconfig pkg-config
  ```
- **Verify:** `pkg-config --exists cairo pangocairo fontconfig && echo OK`
- After installing, reconfigure (`rm -f build/CMakeCache.txt` then `cmake ..` / `make`) so CMake does not keep a forced `HUMID_WITH_HTMLVIEW=OFF`.
- litehtml sources compile as **C++17** static lib `humid_litehtml`; humid core stays C++14 except HTMLVIEW TUs

---

## What’s implemented (feature tree)

### `HTMLVIEW` widget

- Structure class registered in `structure.cpp` (`url` property)
- Factory: `createHtmlView` in `widgetfactory.cpp` / userwindow / dialogwindow
- LinkableText remote string → `setUrl()`
- Fetch via `get_file()` (curl), unique temp HTML deleted after load
- Per-session disk cache under `/tmp/humid-htmlview-cache/sess-…` (removed on container destroy)
- Image surface cache: refcounted for litehtml’s destroy protocol; cap 128
- Viewport paint only (not full-document texture) — scroll with `nvgUpdateImage` when size unchanged
- Explicit cleanup order: document → surfaces → NVG

### Navigation (done)

- Mouse wheel / drag scroll  
- **↑ ↓ PageUp PageDown Home End** (widget must have focus — click first)  
- **TOC / `#fragment` links** (litehtml `on_anchor_click` → jump by `[id=…]`/`[name=…]`)  
- URL with `#fragment` applied after load  
- Same-doc fragment change without full reload  
- **“Top”** overlay button bottom-right when scrolled  
- Drag-scroll cancels accidental link click  

### Demo

```
demo/htmlview/humid/
  SYSTEM.humid          # active_screen: Manual, 1280×800
  PROJECTSETTINGS.humid
  MANUAL.humid          # HTMLVIEW → http://127.0.0.1:8765/4C04_Operators_Manual.html
  ../README.txt
```

### Run demo

```bash
# Terminal 1 — serve manual tree (required for default URL)
cd /Users/mike/src/CW_Simulation/4C04/Docs/operators-manual
python3 -m http.server 8765

# Terminal 2
cd /Users/mike/src/github/humid/demo
../build/humid htmlview/humid
```

Rebuild:

```bash
cd /Users/mike/src/github/humid/build
cmake ..   # should log: HTMLVIEW: enabled (litehtml + Cairo/Pango)
cmake --build . --target humid -- -j4
cmake --build . --target hmifile_check -- -j4
```

---

## hmifile_check (headless) — on production branch

**Problem:** panel error  
`error while loading shared libraries: libglfw.so.3`

**Fix:** checker no longer links NanoGUI/GLFW.

| Change | Detail |
| --- | --- |
| CMake | `hmifile_check` → only cw_client, ZMQ, Boost |
| `anchor_widget.cpp` | GUI-only `WidgetPropertyAnchor` methods |
| `anchor.cpp` | No EditorWidget include |
| `linkmanager.cpp` | Dropped unused `editor.h` / `editorwidget.h` |
| `structure.cpp` | Dropped unused nanogui includes |

Verified on macOS feature build: `otool -L hmifile_check` has **no** glfw/nanogui.

On panel after deploy/rebuild:

```bash
ldd build/hmifile_check | grep -i glfw   # expect empty
./build/hmifile_check Screens/*
```

---

## Not done (product / next)

1. **Commit + push** feature branch HTMLVIEW work.  
2. **Push** `cw-no-ec-tools-compatiblity` (`7a1fa6d`) if panel fleet should get headless check.  
3. **4C04 product wiring** (latproc, not humid-only):
   - CW page value e.g. `Manual` on `V_CoreControl2HMI`
   - `V_ManualURL` (or similar) string property
   - Help entry from settings/menu
   - Return → `M_CoreControl2Panel.Reset` (demo Return is not wired)
4. **Docs host on 4C04** — mini_httpd or static server; full tree (`html` + `css` + `images/`).  
5. **Background fetch** — curl currently on UI thread; can hitch on slow network.  
6. **Panel Cairo/Pango packages** if missing on Ubuntu 18.04 images.  
7. **GLES path** — HTMLVIEW uses Cairo→NVG; confirm on RPi if needed.  
8. Merge strategy: land `hmifile_check` via prod commit first; merge HTMLVIEW when ready (expect CMake/structure conflicts if both diverge).

---

## Memory / crash notes already fixed

- **Segfault on first load:** litehtml Cairo container destroys `get_image()` return value; must `cairo_surface_reference()` (was dangling pointer after first image).  
- **Leaks hygiene:** session disk cache teardown, NVG delete/update, document before container, image cache cap, temp HTML removed after read.

---

## Key files map

| Path | Role |
| --- | --- |
| `src/editorhtmlview.*` | NanoGUI widget |
| `src/htmlview_container.*` | litehtml + curl + Cairo image cache |
| `cmake/Modules/HumidLitehtml.cmake` | Optional litehtml/Cairo build |
| `lib/litehtml/` | Vendored engine |
| `demo/htmlview/humid/` | Runnable demo |
| `doc/html-viewer-bakeoff.md` | Engine comparison |
| `src/anchor_widget.cpp` | GUI anchors (not in hmifile_check) |
| `CMakeLists.txt` | humid + headless hmifile_check |

---

## Suggested first commands in a new session

```bash
cd /Users/mike/src/github/humid
git branch -v
git status -sb
# Feature work:
git checkout feature/html-document-view
# Prod headless check:
git log cw-no-ec-tools-compatiblity -3 --oneline
```

Then either:

- **Ship checker to panels:** push/cherry-pick `7a1fa6d`, rebuild `hmifile_check` on panel.  
- **Continue HTMLVIEW:** commit feature tree, wire 4C04 CW + Return + docs server.

---

## Constraints not to re-litigate

- No external browser / second X client  
- No proprietary embed kits  
- Remote URL is primary content path  
- Return/screen ownership stays in Clockwork for real machines  
- Production OS: old Ubuntu, CMake 3.5+, no assume `cmake -S/-B` only  
- Submodules: don’t casually bump clockwork without branch protocol (`Agents.md`)
