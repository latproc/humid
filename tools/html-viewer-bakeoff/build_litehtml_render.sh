#!/usr/bin/env bash
# Build a one-shot litehtml → PNG tool for the operators-manual bake-off.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
LH="$ROOT/litehtml"
BUILD="$ROOT/litehtml-build"
OUTBIN="$ROOT/litehtml_render"

if [[ ! -f "$BUILD/liblitehtml.a" ]]; then
  mkdir -p "$BUILD"
  (cd "$BUILD" && cmake "$LH" -DLITEHTML_BUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release && cmake --build . -j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)")
fi

CAIRO_CFLAGS=$(pkg-config --cflags cairo pangocairo gdk-3.0 fontconfig)
CAIRO_LIBS=$(pkg-config --libs cairo pangocairo gdk-3.0 fontconfig)

SRC=(
  "$ROOT/litehtml_render_main.cpp"
  "$LH/containers/cairo/render2png.cpp"
  "$LH/containers/cairo/container_cairo.cpp"
  "$LH/containers/cairo/container_cairo_pango.cpp"
  "$LH/containers/cairo/cairo_borders.cpp"
  "$LH/containers/cairo/conic_gradient.cpp"
)

c++ -std=c++17 -O2 \
  -I"$LH/include" -I"$LH/containers/cairo" -I"$LH" \
  $CAIRO_CFLAGS \
  "${SRC[@]}" \
  "$BUILD/liblitehtml.a" \
  "$BUILD/src/gumbo/libgumbo.a" \
  $CAIRO_LIBS \
  -o "$OUTBIN"

echo "Built $OUTBIN"
