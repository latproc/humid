// Minimal litehtml bake-off: HTML file → PNG (local assets via relative paths).
// Build with: tools/html-viewer-bakeoff/build_litehtml_render.sh

#include "containers/cairo/render2png.h"
#include <cstdio>
#include <string>

int main(int argc, char** argv)
{
	if (argc < 3) {
		std::fprintf(stderr, "usage: %s <html_file> <out.png> [width] [height]\n", argv[0]);
		return 2;
	}
	const std::string html = argv[1];
	const std::string out = argv[2];
	const int width = argc > 3 ? std::atoi(argv[3]) : 1240;
	const int height = argc > 4 ? std::atoi(argv[4]) : 800;

	html2png::converter conv(width, height, 96.0, "sans-serif", nullptr);
	if (!conv.to_png(html, out)) {
		std::fprintf(stderr, "render failed: %s\n", html.c_str());
		return 1;
	}
	std::printf("Wrote %s (%dx%d viewport)\n", out.c_str(), width, height);
	return 0;
}
