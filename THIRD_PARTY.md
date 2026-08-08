# Third-party license audit (Humid)

**Date:** 2026-08-08  
**Branch context:** includes HTMLVIEW (litehtml).  
**Purpose:** inventory of code linked into or shipped with Humid, license
status, and required notices. Full license texts live under `LICENSES/`.

This is an engineering audit, not formal legal advice.

---

## Humid project license

| Item | License | Location |
|------|---------|----------|
| Humid sources | BSD-3-style | `LICENSE.txt` |

---

## Linked / shipped components

### Vendored or submodule (compiled into humid / libnanogui)

| Component | How used | License | Notice file | Risk |
|-----------|----------|---------|-------------|------|
| **NanoGUI** | Shared lib `libnanogui` | BSD-3 | `LICENSES/NanoGUI-LICENSE.txt` | OK |
| **GLFW** | Via NanoGUI (`libglfw`) | zlib/libpng | `LICENSES/GLFW-LICENSE.txt` | OK |
| **NanoVG** | Via NanoGUI | zlib | `LICENSES/NanoVG-LICENSE.txt` | OK |
| **libcoro** | Via NanoGUI | BSD-2 | `LICENSES/libcoro-LICENSE.txt` | OK |
| **glad** | Via NanoGUI (generated loader) | MIT | `LICENSES/glad-LICENSE.txt` | OK (text from upstream; no LICENSE in tree) |
| **Eigen** | Headers via NanoGUI | **MPL-2.0** (primary) | `LICENSES/Eigen-MPL2-LICENSE.txt` | OK if unmodified headers; optional `-DEIGEN_MPL2_ONLY` to refuse LGPL snippets |
| **litehtml** | Static `libhumid_litehtml` (HTMLVIEW) | BSD-3 | `LICENSES/litehtml-LICENSE.txt` | OK |
| **gumbo-parser** | Static, inside litehtml | Apache-2.0 | `LICENSES/gumbo-LICENSE.txt` | OK |
| **Roboto Mono** fonts | Bundled under `lib/fonts` | Apache-2.0 | `LICENSES/RobotoMono-LICENSE.txt` | OK |
| **Vendored Clockwork `libcw_client` sources** | Static link | BSD-3-style | `LICENSE.txt` and source headers | OK |

### System / package libraries (dynamic link; typical panel/macOS build)

| Component | How used | License (typical) | Notice in repo | Risk |
|-----------|----------|-------------------|----------------|------|
| **libcurl** | Dynamic | curl / MIT-style | `LICENSES/libcurl-LICENSE.txt` | OK |
| **libzmq** | Dynamic | **MPL-2.0** | `LICENSES/ZeroMQ-MPL2-LICENSE.txt` | OK for dynamic link; keep notice |
| **Boost** | Dynamic | BSL-1.0 | `LICENSES/Boost-LICENSE.txt` | OK |
| **Cairo** | Dynamic (HTMLVIEW) | LGPL-2.1 (common) | System package | OK if dynamic only; ship OS package licenses on panel images |
| **Pango / pangocairo** | Dynamic (HTMLVIEW) | LGPL-2.1+ | System package | Same as Cairo |
| **Fontconfig / FreeType / HarfBuzz / GLib** | Dynamic (HTMLVIEW stack) | MIT / FTL / etc. | System package | Same as Cairo |
| OpenGL / Cocoa / etc. | System frameworks | Apple / platform | N/A | Platform terms |

### Not linked into panel `humid` binary (or optional)

| Component | Notes |
|-----------|--------|
| pybind11 | NanoGUI option; Humid sets `NANOGUI_BUILD_PYTHON=OFF` |
| Clockwork full iod / EtherCAT / SOEM | Separate processes; SOEM/GPL pieces stay in Clockwork plant builds |
| bake-off tools under `tools/html-viewer-bakeoff` | Dev-only; not installed with humid |

---

## HTMLVIEW-specific (new)

| Finding | Status |
|---------|--------|
| litehtml BSD-3 | Compatible; notice added |
| gumbo Apache-2.0 | Compatible; notice added |
| Cairo/Pango stack | Dynamically linked; no vendored copy; panel OS packages carry their own licenses |
| Ultralight / proprietary engines | Not used |

No new copyleft stronger than Apache-2.0 was introduced by HTMLVIEW itself.

---

## Issues and residual risks

### 1. NanoGUI dependency notices were incomplete

Previously `LICENSES/` only carried a NanoGUI top-level BSD text. NanoGUI
also ships **GLFW, NanoVG, Eigen, coro, glad**, which have their own notice
requirements. Those texts are now under `LICENSES/`.

### 2. Eigen MPL-2.0

Eigen is header-only. MPL-2.0 is file-level copyleft: modifications to MPL
files must stay available under MPL. Using **unmodified** Eigen headers as
NanoGUI does is normal and acceptable. For stricter certainty, builds can
define `EIGEN_MPL2_ONLY` (see Eigen `COPYING.README`).

### 3. System LGPL libs (Cairo/Pango)

Dynamic linking to distro Cairo/Pango is the usual LGPL-safe path. Do **not**
statically link LGPL Cairo into humid without complying with LGPL (object
files / relink rights). Current CMake uses pkg-config shared libs.

### 4. Stale NanoGUI notice year

`LICENSES/NanoGUI-LICENSE.txt` had copyright year **2016**; tree copy is
**2017**. Synced to the vendored `lib/nanogui/LICENSE.txt`.

---

## Compatibility matrix (summary)

| Humid (BSD-3) + | Compatible for use? | Notes |
|-----------------|---------------------|--------|
| litehtml BSD-3 | Yes | Notices required |
| gumbo Apache-2.0 | Yes | Notices required |
| NanoGUI BSD-3 + zlib deps | Yes | Notices required |
| Boost BSL | Yes | |
| libcurl MIT-style | Yes | |
| ZeroMQ MPL-2.0 (dynamic) | Yes | |
| Cairo/Pango LGPL (dynamic) | Yes | Prefer shared libs |
| Vendored Clockwork client (BSD-3-style) | Yes | Covered by project license and source headers |

---

## Checklist for binary redistribution

When shipping `humid` / `libnanogui` off-box:

1. Include `LICENSE.txt` and the entire `LICENSES/` directory.
2. Keep HTMLVIEW system packages installed from the OS (Cairo/Pango/etc.).
3. Do not strip `LICENSES/` from panel images under `/opt/humid`.

---

## Files under `LICENSES/`

| File | Covers |
|------|--------|
| `NanoGUI-LICENSE.txt` | NanoGUI |
| `GLFW-LICENSE.txt` | GLFW |
| `NanoVG-LICENSE.txt` | NanoVG |
| `libcoro-LICENSE.txt` | libcoro |
| `glad-LICENSE.txt` | glad loader |
| `Eigen-MPL2-LICENSE.txt` | Eigen (MPL-2.0) |
| `litehtml-LICENSE.txt` | litehtml |
| `gumbo-LICENSE.txt` | gumbo-parser |
| `RobotoMono-LICENSE.txt` | Roboto Mono fonts |
| `libcurl-LICENSE.txt` | libcurl |
| `Boost-LICENSE.txt` | Boost |
| `ZeroMQ-MPL2-LICENSE.txt` | libzmq |

---

## Conclusion

- **HTMLVIEW / litehtml:** no blocking license problem; notices in place.
- **Housekeeping done:** full notice set for NanoGUI transitive deps, Boost,
  ZeroMQ, and sync of NanoGUI text.
