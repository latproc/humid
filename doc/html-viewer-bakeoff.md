# HTML viewer engine bake-off (4C04 operators manual)

**Date:** 2026-08-08  
**Branch:** `feature/html-document-view`  
**Corpus:** `CW_Simulation/4C04/Docs/operators-manual`  
**Serve:** `python3 -m http.server 8765` → `http://127.0.0.1:8765/4C04_Operators_Manual.html`  
**Artifacts:** `tools/html-viewer-bakeoff/out/` (local only; not committed)

## Policy

- **Open source only** (MIT / BSD / Apache). Ultralight rejected.
- Embedded in Humid later (no external window / no WM).
- Content via **URL** from Clockwork when integrated.

## Results

| Metric | Falco (poxk/Falco, MIT) | litehtml (BSD-3) | Chrome headless (ref) |
| --- | --- | --- | --- |
| **Builds** | Yes (`cargo build --release`, ~4.5 min) | Yes (`liblitehtml.a` + cairo/pango) | Yes |
| **Renders manual** | **No** — solid white PNG for URL, local file, simple HTML, and example.com | **Yes** — full document ~1242×26264, tables/TOC/images | **Yes** |
| **Images** | N/A (blank) | Yes (panel screenshots visible) | Yes |
| **Tables / TOC** | N/A | Good | Best |
| **Multi-res** | N/A | Yes (1240 and 1880 width → reflow, same content height) | Yes |
| **Cold time (this Mac)** | ~0.1–0.8 s white output | ~4–5 s full doc PNG | ~1–2 s viewport shot |
| **License** | MIT | BSD-3 + gumbo Apache-2.0 | n/a |
| **Plant / Humid fit** | Rust toolchain + unproven paint path | C++ static lib; need cairo/pango or NVG paint backend in-process | Not for embed |
| **Verdict** | **Fail for product** (until paint works) | **Winner for ship path** | Reference only |

### Falco notes

- CLI: `falco URL --out out.png --width W --height H`.
- Every test produced **100% white** pixels (`#FFFFFFFF`), including:
  - Operators manual over HTTP
  - Same file as local path
  - Trivial `<h1>Hello</h1>` HTML
  - `http://example.com`
- Binary builds; paint path appears broken or non-functional on this host/version.
- README claims are ambitious; **do not** depend on Falco until a known-good render of our manual is demonstrated.
- Optional: re-test after upstream fixes; still carries Rust/FFI cost for C++ Humid.

### litehtml notes

- Tool: `tools/html-viewer-bakeoff/litehtml_render` (+ `build_litehtml_render.sh`).
- Uses upstream cairo/pango container + local relative assets (same tree as remote URL).
- Full-page height grows with content (~26k px) — embed will **scroll a texture/strip**, not one fixed page.
- Fidelity close enough for operator manuals; not pixel-identical to Chrome but readable.
- Production path: vendor litehtml, implement `document_container` with **libcurl** for remote base URL + images (bake-off used local files).

### Chrome notes

- Headless screenshot used only as visual reference.
- Not acceptable for panel (no WM / not embedded).

## Recommendation

1. **Proceed with litehtml** for Humid `HTMLVIEW` on `feature/html-document-view`.
2. **Do not integrate Falco** unless a future version actually paints content and we accept Rust in the build.
3. Next implementation steps:
   - `HTMLVIEW` widget + structure class
   - `remote` → URL string from CW
   - curl-backed resource loader
   - Scrollable blit into NanoGUI
   - Demo screen; then 4C04 `MANUAL.humid` + Return

## How to re-run

```bash
# Terminal 1
cd /path/to/operators-manual && python3 -m http.server 8765

# Falco (expect white until fixed)
export PATH="$HOME/.cargo/bin:$PATH"
./tools/html-viewer-bakeoff/Falco/target/release/falco \
  http://127.0.0.1:8765/4C04_Operators_Manual.html \
  --out tools/html-viewer-bakeoff/out/falco-1240.png --width 1240 --height 800

# litehtml
./tools/html-viewer-bakeoff/build_litehtml_render.sh
./tools/html-viewer-bakeoff/litehtml_render \
  /path/to/4C04_Operators_Manual.html \
  tools/html-viewer-bakeoff/out/litehtml-1240.png 1240 800
```
