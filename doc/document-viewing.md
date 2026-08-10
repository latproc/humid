# Document viewing on Humid (handoff)

**Status (2026-08):** On `master`. Piloted with private ELEC-0297 wiring PDFs
(TwoGrab4Core Manual) offline-converted to PNG and served over HTTP.

| Commit (approx) | What |
| --- | --- |
| `3220b97` | DOCVIEW widget, demo, GLTexture unpack fix, keyboard pan/zoom |
| `55025f7` | Docs point at `llm-rules/tools/pdf_to_web.py` |

---

## Product split

| Content | Widget | Why |
| --- | --- | --- |
| Operators manuals (HTML) | **HTMLVIEW** | litehtml + scroll; no PDF |
| Wiring / CAD multipage drawings | **DOCVIEW** | Sharp zoom into page bitmaps; page flip |
| Single decorative / fixed image | **IMAGE** | Existing; `interactive` locks zoom/pan |

**Not supported on panel:** opening PDF or multipage TIFF in-process. Convert
offline; ship static `page-00N.png` (and optional HTML catalog).

---

## How DOCVIEW is implemented

### Class and registration

| Piece | Location |
| --- | --- |
| Widget | `src/editordocview.h`, `src/editordocview.cpp` |
| Builtin class | `Structure::loadBuiltins()` → `"DOCVIEW"` |
| Property map | `Source`→`source`, `Page`→`page`, `Pages`→`pages`, `Scale`, `Interactive` |
| Factory create | `createDocView()` in `src/widgetfactory.cpp` |
| Screen load | `userwindow.cpp`, `dialogwindow.cpp` |
| Palette / starter | `structureswindow.cpp`, `factorybuttons.cpp` |
| CMake | `src/editordocview.cpp` / `.h` listed on `humid` target |
| Remotes | `LinkableText` → `setSource()`; `LinkableNumber` → `setPage()` |

`EditorDocView` **extends** `nanogui::ImageView` (same GL textured quad path as
IMAGE) plus `EditorWidget` for editor properties and remotes.

### Page URL model

1. Operator/screen sets **`source`** to a document **directory** URL (trailing
   slash optional), e.g. `http://host:8767/elec-0297-…/`.
2. Humid builds  
   `source + "page-" + zero-padded 3-digit page + ".png"`  
   → `…/page-001.png`, `…/page-002.png`, …
3. Load uses existing `EditorGUI::getImageId()`:
   - if `http://…` → download into cwd-relative `cache/` (see `.gitignore`)
   - decode PNG/JPEG via `GLTexture` / `stb_image`
   - bind as OpenGL texture; only **one page texture** held for the widget
4. **`pages`** is the known page count (must match converted set). **`page`** is
   1-based current index.

Local relative paths work the same way if the file exists under the process
cwd (no `http://` prefix).

### Operator controls (keyboard-first)

Mouse **drag pan** and **wheel zoom are disabled** (panel / trackpad pan was
awkward). Click once on the widget so it takes focus.

| Input | Action |
| --- | --- |
| Arrows | Pan (~100 px; **Shift** ~220; **Ctrl** ~140) |
| `+` / `-` (and keypad) | Zoom about widget centre |
| `F` or `0` | Fit |
| `C` | Centre |
| PgUp / PgDn or `[` / `]` | Previous / next page |
| Home / End | First / last page |
| On-screen **Prev** / **Next** / **Fit** | Touch-friendly page / fit |

Chrome is drawn in `drawChrome()` (bottom of widget). Grid/pixel-info overlays
are turned off for drawings.

### Shared IMAGE texture fix (important)

Wiring A4 pages at 150 dpi are often **odd pixel widths** (e.g. 1754). Old
`GLTexture::load` uploaded **RGB** with default `GL_UNPACK_ALIGNMENT = 4`, which
**sheared / “damaged”** the image when `width × 3` is not a multiple of 4.

**Fix** in `src/gltexture.cpp` (IMAGE **and** DOCVIEW):

- Decode as **RGBA** (`stbi` `req_comp = 4`)
- Set `GL_UNPACK_ALIGNMENT` to **1** around `glTexImage2D`

If IMAGE drawings still look diagonal on an old binary, rebuild against this
loader.

### Demo project

```text
demo/docview/
  README.txt
  humid/          # SYSTEM, PROJECTSETTINGS, DRAWINGS
  pages/sample-drawing/page-001.png … page-003.png   # public 8-bit RGB sample
```

Default screen points at `http://127.0.0.1:8767/sample-drawing/` (not private
plant PDFs).

---

## How to use DOCVIEW on a plant screen

### 1. Convert PDFs offline (dev machine)

```sh
# Prefer llm-rules copy (SVN latproc); plant Docs/schematics may keep a twin.
python3 /path/to/llm-rules/tools/pdf_to_web.py \
  --width 1240 --height 700 --dpi 150 \
  --source /path/to/private/pdfs \
  --web /path/to/served/web
```

- **150 dpi** recommended for electrical linework on J2900-class panels.
- Needs `pdftoppm` (poppler), or `pdftocairo`, or Ghostscript.
- Output: `web/<doc-id>/page-001.png` … and optional HTML index (for HTMLVIEW
  menus only).
- **Do not commit private PDFs into public humid.**

### 2. Serve the tree over HTTP

```sh
cd /path/to/served/web
python3 -m http.server 8767
# or plant content server / reverse proxy on the panel network
```

### 3. Place a DOCVIEW on a screen

Editor palette: **DOCVIEW**, or hand-authored `.humid`:

```text
Doc DOCVIEW(
  source: "http://docs-host:8767/elec-0297-2g-4c-bechoff-2018-wiring-page-1-6/",
  page: 1,
  pages: 6,
  interactive: 1,
  border: 1,
  pos_x: 10, pos_y: 56, width: 1260, height: 660);
```

| Property | Meaning |
| --- | --- |
| `source` | Base URL (or directory) of the pack |
| `page` | Current 1-based page |
| `pages` | Total page count |
| `interactive` | Keyboard pan/zoom (default true) |
| `scale` | ImageView scale after load (fit is applied on load) |

### 4. Optional Clockwork links

- **String remote** on the widget → updates `source` (switch drawing pack).
- **Numeric remote** via `LinkableNumber` → updates `page`.

### 5. Smoke test

```sh
# Public demo
cd demo/docview/pages && python3 -m http.server 8767
cd demo && ../build/humid docview/humid
```

Expect: page image, **1 / N** label, Prev/Next/Fit, keyboard pan/zoom after
click. Failures show “Load failed” (check server, `http://` only in `isURL`,
8-bit PNG, rebuild after texture fix).

---

## HTMLVIEW vs DOCVIEW (when to pick which)

| Need | Use |
| --- | --- |
| Long HTML manual, TOC, reflow text | HTMLVIEW |
| Multipage wiring with sharp zoom | DOCVIEW |
| Single static logo / photo | IMAGE (`interactive` if zoom wanted) |

HTMLVIEW still scrolls long content; it does **not** replace DOCVIEW zoom for
CAD. Do not put JS zoom in generated schematic HTML (litehtml ignores it).

---

## Hardware notes

- J2900-class, newer **Wayland** panels, and at least one **Pi 4** (GLES).
- One page resident; avoid 300 dpi unless proven on target.
- Page change = HTTP (or cache hit) + texture upload — fine every few seconds.

---

## Related files

| Area | Path |
| --- | --- |
| This handoff | `doc/document-viewing.md` |
| Demo how-to | `demo/docview/README.txt` |
| HTMLVIEW bake-off history | `doc/html-viewer-bakeoff.md` |
| Converter (agents / plant) | `llm-rules/tools/pdf_to_web.py` (SVN latproc) |
| Converter notes | `llm-rules/TOOLS.md`, `llm-rules/humid/AGENT_GUIDE.md` |
| Runtime image cache | cwd `cache/` (gitignored) |

## Out of scope (v1)

- In-panel PDF rendering
- Multipage TIFF
- Mouse pan / wheel zoom on DOCVIEW
- HTMLVIEW document-level pinch zoom (use DOCVIEW for drawings)
