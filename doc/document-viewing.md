# Document viewing on Humid panels

Two complementary paths:

| Content | Viewer | Source |
| --- | --- | --- |
| **Operators manuals** (HTML) | **HTMLVIEW** | HTML + CSS + images over HTTP |
| **Wiring / CAD drawings** (from PDF masters) | **DOCVIEW** | Per-page PNG over HTTP |

Humid does **not** open PDF or multipage TIFF on the panel. Convert offline; serve static assets.

## Hardware

Fleet includes older **J2900-class** machines, newer **Wayland** panels, and at least one **Raspberry Pi 4** (GLES). Design rules:

- One page texture resident at a time
- ~**150 dpi** A4 PNG default (~1750×1240 landscape); avoid 300 dpi on small panels
- Zoom/pan on a static GL texture (cheap after load)
- Page flip = HTTP fetch (cached under `cache/`) + texture upload

## Offline conversion (dev machine)

Use plant tooling, e.g. `4C04/Docs/schematics/pdf_to_web.py`:

```sh
# Example: multipage electrical PDF → page-001.png …
pdftoppm -png -r 150 drawing.pdf out/page
# rename to page-001.png … or use pdf_to_web.py --dpi 150 --width 1240 --height 700
```

Private PDFs stay in plant trees (e.g. TwoGrab4Core `Docs/Manual`); never commit them into public humid.

## Serve over HTTP

All page files are pulled by humid over the network (same pattern as HTMLVIEW manuals):

```sh
cd /path/to/web-or-pages-root
python3 -m http.server 8767
```

DOCVIEW `source` is the **document directory URL**:

```text
http://host:8767/elec-0297-2g-4c-…/
  page-001.png
  page-002.png
  …
```

Humid builds `source + "page-00N.png"` and loads via `EditorGUI::getImageId` (HTTP → local `cache/`).

## DOCVIEW properties

| Property | Meaning |
| --- | --- |
| `source` | Base URL (or directory) for the drawing pack |
| `page` | Current 1-based page |
| `pages` | Total page count |
| `interactive` | Zoom/pan enabled (default true) |
| `scale` | ImageView scale (usually leave fit after load) |

Chrome: **Prev**, **Next**, **Fit**, page label.

Keyboard (primary — mouse pan/wheel disabled):

| Keys | Action |
| --- | --- |
| Arrows | Pan (Shift = larger) |
| `+` / `-` | Zoom in / out |
| `F` or `0` | Fit |
| `C` | Centre |
| PgUp / PgDn or `[` / `]` | Previous / next page |
| Home / End | First / last page |

## HTML manuals

Continue using HTMLVIEW for HTML-based operators manuals. Optional HTML catalog of drawings can link into screens that host DOCVIEW; zoom belongs in DOCVIEW, not JavaScript (litehtml has no JS).

## Demo

See [`demo/docview/README.txt`](../demo/docview/README.txt).
