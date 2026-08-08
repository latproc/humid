//
//  editorhtmlview.cpp
//  Project: humid
//

#include "editorhtmlview.h"
#include "htmlview_container.h"
#include "editor.h"
#include "propertyformhelper.h"
#include "structure.h"
#include "helper.h"
#include "linkableobject.h"

#include <litehtml.h>
#include <cairo.h>
#include <nanogui/screen.h>
#include <nanogui/opengl.h>

#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <chrono>
#include <cstdio>

namespace fs = boost::filesystem;
using Clock = std::chrono::steady_clock;

namespace {
// Escape for use inside CSS attribute selectors.
std::string cssEscapeAttr(const std::string &s) {
	std::string out;
	out.reserve(s.size() + 8);
	for (char c : s) {
		if (c == '\\' || c == '"' || c == '\'')
			out.push_back('\\');
		out.push_back(c);
	}
	return out;
}

std::string stripFragment(const std::string &url, std::string *fragment_out) {
	auto hash = url.find('#');
	if (hash == std::string::npos) {
		if (fragment_out)
			fragment_out->clear();
		return url;
	}
	if (fragment_out)
		*fragment_out = url.substr(hash + 1);
	return url.substr(0, hash);
}

bool sameDocumentUrl(const std::string &a, const std::string &b) {
	std::string fa, fb;
	return stripFragment(a, &fa) == stripFragment(b, &fb);
}
} // namespace

const std::map<std::string, std::string> &EditorHtmlView::property_map() const {
	auto structure_class = findClass("HTMLVIEW");
	assert(structure_class);
	return structure_class->property_map();
}

const std::map<std::string, std::string> &EditorHtmlView::reverse_property_map() const {
	auto structure_class = findClass("HTMLVIEW");
	assert(structure_class);
	return structure_class->reverse_property_map();
}

EditorHtmlView::EditorHtmlView(NamedObject *owner, Widget *parent, const std::string nam, LinkableProperty *lp)
	: Widget(parent), EditorWidget(owner, "HTMLVIEW", nam, this, lp), m_container(new HtmlViewContainer()) {
	m_status = "No URL";
	m_container->setAnchorClickCallback([this](const std::string &href) { onAnchorClick(href); });
}

void EditorHtmlView::releaseNvgImage() {
	if (m_nvg_image >= 0) {
		if (screen())
			nvgDeleteImage(screen()->nvgContext(), m_nvg_image);
		m_nvg_image = -1;
	}
}

void EditorHtmlView::releaseDocument() {
	// Document must be destroyed before image surfaces (container is non-owning to litehtml).
	m_doc.reset();
	m_content_height = 0;
	m_content_width = 0;
	m_scroll_y = 0;
	// Drop in-flight HTML held between Fetch and Layout stages.
	m_pending_html.clear();
	m_pending_html.shrink_to_fit();
	m_pending_base.clear();
	if (m_container) {
		m_container->clearImageSurfaces();
		// Session URL→path map can grow if many manuals are opened; disk cache remains.
		m_container->clearMemoryMaps();
	}
	releaseNvgImage();
	m_rgba.clear();
	m_rgba.shrink_to_fit();
	m_rgba_w = m_rgba_h = 0;
	m_need_paint = true;
	m_mouse_down = false;
	m_dragging = false;
	m_press_on_top = false;
	m_press_x = m_press_y = -1;
}

EditorHtmlView::~EditorHtmlView() {
	// Abort multi-frame load so draw/advanceLoad cannot run after teardown.
	m_load_phase = LoadPhase::Idle;
	m_load_busy = false;
	// Drop callback into this before destroying the container.
	if (m_container)
		m_container->setAnchorClickCallback(nullptr);
	releaseDocument();
	m_container.reset();
}

long EditorHtmlView::msSince(Clock::time_point t0) {
	return (long)std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - t0).count();
}

void EditorHtmlView::requestNextFrame() {
	// Humid only paints when ClockworkClient::needs_frame_redraw is set
	// (see skeleton.cpp drawAll). Prefer EDITOR gui; fall back if early init.
	if (EDITOR && EDITOR->gui())
		EDITOR->gui()->requestRedraw();
}

void EditorHtmlView::setUrl(const std::string &url, bool force_reload) {
	std::string frag;
	std::string base = stripFragment(url, &frag);
	std::string cur_base = stripFragment(m_url, nullptr);

	// Same document, only fragment changed → jump without reload.
	if (!force_reload && !m_url.empty() && base == cur_base && m_doc && !m_load_busy) {
		m_url = url;
		m_pending_fragment = frag;
		if (frag.empty())
			jumpToTop();
		else
			jumpToFragment(frag);
		return;
	}

	if (!force_reload && url == m_url && !m_load_busy)
		return;

	m_url = url;
	m_pending_fragment = frag;
	m_scroll_y = 0;
	requestReload();
}

void EditorHtmlView::requestReload() {
	if (m_url.empty()) {
		m_status = "No URL";
		m_status_detail.clear();
		m_load_phase = LoadPhase::Idle;
		m_load_busy = false;
		m_pending_html.clear();
		m_pending_base.clear();
		return;
	}
	// Multi-frame load: paint "Loading…" this frame before any network/layout work.
	// releaseDocument() also clears any in-flight m_pending_html from a prior load.
	std::string keep_frag = m_pending_fragment;
	releaseDocument();
	m_pending_fragment = keep_frag;

	m_load_phase = LoadPhase::Pending;
	m_load_busy = true;
	m_status = "Loading document…";
	m_status_detail = "Please wait";
	m_timing_line.clear();
	m_ms_fetch_html = m_ms_prefetch = m_ms_parse_layout = m_ms_first_paint = 0;
	m_prefetch_count = 0;
	m_load_t0 = Clock::now();
	requestNextFrame();
}

void EditorHtmlView::jumpToTop() {
	setScrollY(0);
}

bool EditorHtmlView::jumpToFragment(const std::string &fragment) {
	if (!m_doc || !m_doc->root() || fragment.empty())
		return false;

	const std::string esc = cssEscapeAttr(fragment);
	// Same approach as litehtml's web_page::show_fragment (pandoc ids + name=).
	litehtml::element::ptr el =
		m_doc->root()->select_one(":is([id=\"" + esc + "\"],[name=\"" + esc + "\"])");
	if (!el)
		el = m_doc->root()->select_one("[id=\"" + esc + "\"],[name=\"" + esc + "\"]");
	if (!el)
		el = m_doc->root()->select_one("#" + esc);
	if (!el)
		return false;

	litehtml::position pos = el->get_placement();
	// pixel_t converts to float; place heading near the top of the viewport.
	const int y = (int)std::lround(pos.top().value());
	setScrollY(y);
	return true;
}

void EditorHtmlView::applyPendingFragment() {
	if (m_pending_fragment.empty())
		return;
	if (jumpToFragment(m_pending_fragment))
		m_pending_fragment.clear();
}

void EditorHtmlView::onAnchorClick(const std::string &href) {
	if (href.empty())
		return;

	// Pure fragment: #section
	if (href[0] == '#') {
		std::string frag = href.substr(1);
		m_pending_fragment = frag;
		if (!jumpToFragment(frag))
			std::cerr << "HTMLVIEW: fragment not found: " << frag << "\n";
		return;
	}

	// Resolve relative links against current document base
	std::string resolved = href;
	if (href.find("http://") != 0 && href.find("https://") != 0 && href.find("file:") != 0) {
		std::string base = stripFragment(m_url, nullptr);
		// simple join
		if (!base.empty()) {
			auto slash = base.find_last_of('/');
			if (slash != std::string::npos)
				resolved = base.substr(0, slash + 1) + href;
		}
	}

	std::string frag;
	std::string bare = stripFragment(resolved, &frag);
	std::string cur = stripFragment(m_url, nullptr);
	if (bare == cur || bare.empty()) {
		// Same page fragment
		if (frag.empty())
			jumpToTop();
		else {
			m_pending_fragment = frag;
			if (!jumpToFragment(frag))
				std::cerr << "HTMLVIEW: fragment not found: " << frag << "\n";
		}
		return;
	}

	// Different document
	setUrl(resolved, true);
}

void EditorHtmlView::runFetchStage() {
	m_pending_html.clear();
	m_pending_base.clear();

	std::string frag_from_url;
	std::string fetch_url = stripFragment(m_url, &frag_from_url);
	if (m_pending_fragment.empty() && !frag_from_url.empty())
		m_pending_fragment = frag_from_url;

	m_status = "Loading document…";
	m_status_detail = "Fetching / validating cache…";

	const auto t_html0 = Clock::now();
	std::string html;
	std::string base = m_url;

	if (fetch_url.find("http://") == 0 || fetch_url.find("https://") == 0) {
		std::string local;
		if (!m_container->ensureLocalFile(fetch_url, local)) {
			m_status = "Fetch failed";
			m_status_detail = fetch_url;
			m_load_phase = LoadPhase::Idle;
			m_load_busy = false;
			std::cerr << "HTMLVIEW: ensureLocalFile failed for " << fetch_url << "\n";
			return;
		}
		std::ifstream in(local.c_str(), std::ios::binary);
		if (!in) {
			m_status = "Open failed";
			m_status_detail = local;
			m_load_phase = LoadPhase::Idle;
			m_load_busy = false;
			return;
		}
		std::ostringstream ss;
		ss << in.rdbuf();
		html = ss.str();
		base = fetch_url;
	} else {
		std::string path = fetch_url;
		if (path.find("file://") == 0)
			path = path.substr(7);
		std::ifstream in(path.c_str(), std::ios::binary);
		if (!in) {
			m_status = "Open failed";
			m_status_detail = path;
			m_load_phase = LoadPhase::Idle;
			m_load_busy = false;
			return;
		}
		std::ostringstream ss;
		ss << in.rdbuf();
		html = ss.str();
		base = path;
	}
	m_ms_fetch_html = msSince(t_html0);

	if (html.empty()) {
		m_status = "Empty document";
		m_status_detail.clear();
		m_load_phase = LoadPhase::Idle;
		m_load_busy = false;
		return;
	}

	m_container->setBaseUrl(base);
	const int vw = std::max(1, width() > 0 ? width() : 800);
	const int vh = std::max(1, height() > 0 ? height() : 600);
	m_container->setViewport(vw, vh);

	m_prefetch_count = 0;
	const auto t_pf0 = Clock::now();
	if (fetch_url.find("http://") == 0 || fetch_url.find("https://") == 0) {
		std::vector<std::string> assets = HtmlViewContainer::collectAssetUrls(html, base);
		m_prefetch_count = (int)assets.size();
		if (!assets.empty()) {
			m_status_detail = "Fetching " + std::to_string(assets.size()) + " assets (parallel)…";
			std::cerr << "HTMLVIEW: prefetching " << assets.size() << " assets (parallel)\n";
			m_container->prefetchUrls(assets, 8);
		}
	}
	m_ms_prefetch = msSince(t_pf0);

	std::cerr << "HTMLVIEW timing: fetch_html=" << m_ms_fetch_html << "ms prefetch=" << m_ms_prefetch
			  << "ms (" << m_prefetch_count << " assets) url=" << fetch_url << "\n";

	m_pending_html = std::move(html);
	m_pending_base = base;
	m_status = "Rendering document…";
	m_status_detail = "Layout may take several seconds (litehtml)…";
	m_load_phase = LoadPhase::ShowRendering;
}

void EditorHtmlView::runLayoutStage() {
	const std::string html = std::move(m_pending_html);
	m_pending_html.clear();
	m_pending_html.shrink_to_fit();
	const std::string base = m_pending_base;
	m_pending_base.clear();

	if (html.empty()) {
		m_status = "Empty document";
		m_load_phase = LoadPhase::Idle;
		m_load_busy = false;
		return;
	}

	m_status = "Rendering document…";
	m_status_detail = "Parsing and laying out…";

	const int vw = std::max(1, width() > 0 ? width() : 800);
	const int vh = std::max(1, height() > 0 ? height() : 600);
	m_container->setViewport(vw, vh);
	// Do not call setBaseUrl() here: it clears decoded image surfaces. Base was set
	// in the fetch stage; only refresh if it somehow differs.
	if (m_container->baseUrl() != base)
		m_container->setBaseUrl(base);

	const auto t_lay0 = Clock::now();
	try {
		// Drop any previous document before building a new one.
		m_doc.reset();
		if (m_container)
			m_container->clearImageSurfaces();

		m_doc = litehtml::document::createFromString(html.c_str(), m_container.get());
		// `html` goes out of scope at end of function; litehtml has its own tree by now.
		if (!m_doc) {
			m_status = "Parse failed";
			m_status_detail.clear();
			m_load_phase = LoadPhase::Idle;
			m_load_busy = false;
			if (m_container)
				m_container->clearImageSurfaces();
			return;
		}
		m_content_width = (int)m_doc->render(vw);
		m_content_height = (int)m_doc->height();
		m_ms_parse_layout = msSince(t_lay0);

		std::cerr << "HTMLVIEW timing: parse_layout=" << m_ms_parse_layout
				  << "ms content=" << m_content_width << "x" << m_content_height << "\n";

		m_status = "OK";
		m_status_detail.clear();
		m_need_paint = true;
		applyPendingFragment();
		m_load_phase = LoadPhase::Idle;
		// first_paint timed in paintViewport; load_busy cleared after first paint log
	} catch (const std::exception &e) {
		m_ms_parse_layout = msSince(t_lay0);
		m_status = std::string("Error: ") + e.what();
		m_status_detail.clear();
		m_doc.reset();
		if (m_container)
			m_container->clearImageSurfaces();
		m_load_phase = LoadPhase::Idle;
		m_load_busy = false;
	} catch (...) {
		m_ms_parse_layout = msSince(t_lay0);
		m_status = "Render error";
		m_status_detail.clear();
		m_doc.reset();
		if (m_container)
			m_container->clearImageSurfaces();
		m_load_phase = LoadPhase::Idle;
		m_load_busy = false;
	}
}

void EditorHtmlView::advanceLoad(NVGcontext * /*ctx*/) {
	switch (m_load_phase) {
	case LoadPhase::Pending:
		// This frame only shows the Loading panel (painted after advanceLoad returns).
		m_status = "Loading document…";
		m_status_detail = "Please wait";
		m_load_phase = LoadPhase::Fetch;
		requestNextFrame();
		break;
	case LoadPhase::Fetch:
		runFetchStage();
		requestNextFrame();
		break;
	case LoadPhase::ShowRendering:
		// This frame only shows the Rendering panel before the heavy layout frame.
		m_status = "Rendering document…";
		m_status_detail = "Layout may take several seconds (litehtml)…";
		m_load_phase = LoadPhase::Layout;
		requestNextFrame();
		break;
	case LoadPhase::Layout:
		runLayoutStage();
		requestNextFrame();
		break;
	case LoadPhase::Idle:
	default:
		break;
	}
}

void EditorHtmlView::setScrollY(int y) {
	int max_scroll = std::max(0, m_content_height - std::max(1, height()));
	y = std::max(0, std::min(y, max_scroll));
	if (y != m_scroll_y) {
		m_scroll_y = y;
		m_need_paint = true;
	}
}

nanogui::Vector2i EditorHtmlView::docCoords(const nanogui::Vector2i &widget_p) const {
	return nanogui::Vector2i(widget_p.x(), widget_p.y() + m_scroll_y);
}

nanogui::Vector4i EditorHtmlView::topButtonRect() const {
	const int bw = 72;
	const int bh = 32;
	const int margin = 10;
	return nanogui::Vector4i(width() - bw - margin, height() - bh - margin, bw, bh);
}

bool EditorHtmlView::hitTopButton(const nanogui::Vector2i &p) const {
	if (m_scroll_y <= 0)
		return false;
	auto r = topButtonRect();
	return p.x() >= r.x() && p.x() < r.x() + r.z() && p.y() >= r.y() && p.y() < r.y() + r.w();
}

void EditorHtmlView::paintViewport() {
	if (!m_doc || width() <= 0 || height() <= 0) {
		// Avoid stuck m_load_busy if layout finished before the widget has a size.
		if (m_doc && m_load_busy && m_status == "OK")
			requestNextFrame();
		return;
	}

	const bool log_first_paint = m_load_busy;
	const auto t_paint0 = Clock::now();

	const int w = width();
	const int h = height();
	m_container->setViewport(w, h);

	if (m_content_width != w) {
		const auto t_reflow0 = Clock::now();
		m_content_width = (int)m_doc->render(w);
		m_content_height = (int)m_doc->height();
		int max_scroll = std::max(0, m_content_height - h);
		if (m_scroll_y > max_scroll)
			m_scroll_y = max_scroll;
		applyPendingFragment();
		std::cerr << "HTMLVIEW timing: reflow=" << msSince(t_reflow0) << "ms width=" << w << "\n";
	}

	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
	if (!surface || cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
		if (surface)
			cairo_surface_destroy(surface);
		m_status = "Cairo surface failed";
		m_load_busy = false;
		return;
	}
	cairo_t *cr = cairo_create(surface);
	if (!cr) {
		cairo_surface_destroy(surface);
		m_status = "Cairo context failed";
		m_load_busy = false;
		return;
	}
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);

	litehtml::position clip(0, 0, w, h);
	m_doc->draw((litehtml::uint_ptr)cr, 0, -m_scroll_y, &clip);

	cairo_surface_flush(surface);
	unsigned char *data = cairo_image_surface_get_data(surface);
	const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 4;
	m_rgba.resize(nbytes);
	for (int i = 0; i < w * h; ++i) {
		unsigned char b = data[i * 4 + 0];
		unsigned char g = data[i * 4 + 1];
		unsigned char r = data[i * 4 + 2];
		unsigned char a = data[i * 4 + 3];
		m_rgba[i * 4 + 0] = r;
		m_rgba[i * 4 + 1] = g;
		m_rgba[i * 4 + 2] = b;
		m_rgba[i * 4 + 3] = a ? a : 255;
	}

	cairo_destroy(cr);
	cairo_surface_destroy(surface);

	if (screen()) {
		NVGcontext *vg = screen()->nvgContext();
		if (m_nvg_image >= 0 && m_rgba_w == w && m_rgba_h == h) {
			nvgUpdateImage(vg, m_nvg_image, m_rgba.data());
		} else {
			releaseNvgImage();
			m_nvg_image = nvgCreateImageRGBA(vg, w, h, 0, m_rgba.data());
		}
	} else {
		releaseNvgImage();
	}

	m_rgba_w = w;
	m_rgba_h = h;
	m_need_paint = false;

	const long paint_ms = msSince(t_paint0);
	if (log_first_paint) {
		m_ms_first_paint = paint_ms;
		const long total = msSince(m_load_t0);
		std::ostringstream line;
		line << "fetch_html=" << m_ms_fetch_html << "ms"
			 << " prefetch=" << m_ms_prefetch << "ms(" << m_prefetch_count << ")"
			 << " parse_layout=" << m_ms_parse_layout << "ms"
			 << " first_paint=" << m_ms_first_paint << "ms"
			 << " total=" << total << "ms";
		m_timing_line = line.str();
		std::cerr << "HTMLVIEW timing: " << m_timing_line << " url=" << m_url << "\n";
		m_load_busy = false;
	}
}

void EditorHtmlView::drawStatusPanel(NVGcontext *ctx) {
	const float x = (float)mPos.x();
	const float y = (float)mPos.y();
	const float w = (float)mSize.x();
	const float h = (float)mSize.y();

	// Background
	nvgBeginPath(ctx);
	nvgRect(ctx, x, y, w, h);
	nvgFillColor(ctx, nvgRGBA(245, 247, 250, 255));
	nvgFill(ctx);

	// Center card
	const float card_w = std::min(w - 40.f, 520.f);
	const float card_h = 160.f;
	const float cx = x + (w - card_w) * 0.5f;
	const float cy = y + (h - card_h) * 0.5f;

	nvgBeginPath(ctx);
	nvgRoundedRect(ctx, cx, cy, card_w, card_h, 8.f);
	nvgFillColor(ctx, nvgRGBA(255, 255, 255, 255));
	nvgFill(ctx);
	nvgStrokeWidth(ctx, 1.f);
	nvgStrokeColor(ctx, nvgRGBA(180, 190, 200, 255));
	nvgStroke(ctx);

	// Accent bar
	nvgBeginPath(ctx);
	nvgRoundedRect(ctx, cx, cy, 6.f, card_h, 3.f);
	const bool rendering =
		m_load_phase == LoadPhase::ShowRendering || m_load_phase == LoadPhase::Layout ||
		(m_status.find("Rendering") != std::string::npos);
	nvgFillColor(ctx, rendering ? nvgRGBA(200, 120, 40, 255) : nvgRGBA(30, 100, 170, 255));
	nvgFill(ctx);

	nvgFontFace(ctx, "sans-bold");
	nvgFontSize(ctx, 22.f);
	nvgFillColor(ctx, nvgRGBA(30, 40, 50, 255));
	nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
	const std::string title = m_status.empty() ? std::string("HTMLVIEW") : m_status;
	nvgText(ctx, cx + 24.f, cy + 24.f, title.c_str(), nullptr);

	nvgFontFace(ctx, "sans");
	nvgFontSize(ctx, 16.f);
	nvgFillColor(ctx, nvgRGBA(70, 80, 90, 255));
	if (!m_status_detail.empty())
		nvgTextBox(ctx, cx + 24.f, cy + 56.f, card_w - 40.f, m_status_detail.c_str(), nullptr);

	// Elapsed while busy
	if (m_load_busy) {
		const long elapsed = msSince(m_load_t0);
		char buf[64];
		std::snprintf(buf, sizeof(buf), "Elapsed: %.1f s", elapsed / 1000.0);
		nvgFontSize(ctx, 14.f);
		nvgFillColor(ctx, nvgRGBA(100, 110, 120, 255));
		nvgText(ctx, cx + 24.f, cy + 100.f, buf, nullptr);
	}

	// URL footer
	if (!m_url.empty()) {
		nvgFontSize(ctx, 12.f);
		nvgFillColor(ctx, nvgRGBA(120, 130, 140, 255));
		nvgTextBox(ctx, cx + 24.f, cy + 124.f, card_w - 40.f, m_url.c_str(), nullptr);
	}

	// Last timing strip at bottom of widget (after a completed load, on error screens too)
	if (!m_timing_line.empty() && !m_load_busy) {
		nvgFontSize(ctx, 11.f);
		nvgFillColor(ctx, nvgRGBA(90, 100, 110, 255));
		nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);
		nvgTextBox(ctx, x + 10.f, y + h - 8.f, w - 20.f, m_timing_line.c_str(), nullptr);
	}
}

void EditorHtmlView::draw(NVGcontext *ctx) {
	Widget::draw(ctx);

	// Advance multi-frame load *before* painting so Fetch/Layout run after a
	// frame that already displayed the previous status (Loading / Rendering).
	if (m_load_phase != LoadPhase::Idle)
		advanceLoad(ctx);

	// Ready to show document (may still need first paint this frame).
	const bool ready = m_doc && m_status == "OK" && m_load_phase == LoadPhase::Idle;

	if (ready && (m_need_paint || m_rgba_w != width() || m_rgba_h != height()))
		paintViewport();

	if (ready && m_nvg_image >= 0) {
		nvgBeginPath(ctx);
		nvgRect(ctx, mPos.x(), mPos.y(), (float)mSize.x(), (float)mSize.y());
		nvgFillColor(ctx, nvgRGBA(255, 255, 255, 255));
		nvgFill(ctx);

		NVGpaint img = nvgImagePattern(ctx, mPos.x(), mPos.y(), (float)mSize.x(), (float)mSize.y(), 0.f,
									   m_nvg_image, 1.f);
		nvgBeginPath(ctx);
		nvgRect(ctx, mPos.x(), mPos.y(), (float)mSize.x(), (float)mSize.y());
		nvgFillPaint(ctx, img);
		nvgFill(ctx);

		// After a timed load, keep the breakdown visible briefly at the bottom.
		if (!m_timing_line.empty()) {
			nvgBeginPath(ctx);
			nvgRect(ctx, mPos.x(), mPos.y() + mSize.y() - 22.f, (float)mSize.x(), 22.f);
			nvgFillColor(ctx, nvgRGBA(255, 255, 255, 200));
			nvgFill(ctx);
			nvgFontFace(ctx, "sans");
			nvgFontSize(ctx, 11.f);
			nvgFillColor(ctx, nvgRGBA(50, 60, 70, 255));
			nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);
			nvgText(ctx, mPos.x() + 8.f, mPos.y() + mSize.y() - 6.f, m_timing_line.c_str(), nullptr);
		}
	} else {
		drawStatusPanel(ctx);
	}

	// Jump-to-top control when scrolled
	if (ready && m_nvg_image >= 0 && m_scroll_y > 0) {
		auto r = topButtonRect();
		float x = mPos.x() + r.x();
		float y = mPos.y() + r.y();
		float bw = (float)r.z();
		float bh = (float)r.w();
		nvgBeginPath(ctx);
		nvgRoundedRect(ctx, x, y, bw, bh, 4.f);
		nvgFillColor(ctx, nvgRGBA(30, 80, 140, 220));
		nvgFill(ctx);
		nvgFontSize(ctx, 15.f);
		nvgFontFace(ctx, "sans-bold");
		nvgFillColor(ctx, nvgRGBA(255, 255, 255, 255));
		nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgText(ctx, x + bw * 0.5f, y + bh * 0.5f, "Top", nullptr);
	}

	if (border > 0) {
		nvgBeginPath(ctx);
		nvgStrokeWidth(ctx, (float)border);
		nvgRect(ctx, mPos.x() + 0.5f, mPos.y() + 0.5f, (float)mSize.x() - 1, (float)mSize.y() - 1);
		nvgStrokeColor(ctx, nvgRGBA(80, 80, 80, 255));
		nvgStroke(ctx);
	}

	if (mSelected)
		drawSelectionBorder(ctx, mPos, mSize);
	else if (EDITOR && EDITOR->isEditMode())
		drawElementBorder(ctx, mPos, mSize);
}

void EditorHtmlView::fireDocumentClick(const nanogui::Vector2i &local_p) {
	if (!m_doc)
		return;
	// Atomic click at release position — do not arm litehtml on press, or a
	// later drag-scroll will either steal the click or jump via a stale target.
	auto d = docCoords(local_p);
	m_doc->on_mouse_over(d.x(), d.y(), local_p.x(), local_p.y(), [](const litehtml::position &) {});
	m_doc->on_lbutton_down(d.x(), d.y(), local_p.x(), local_p.y(), [](const litehtml::position &) {});
	m_doc->on_lbutton_up(d.x(), d.y(), local_p.x(), local_p.y(), [](const litehtml::position &) {});
	m_need_paint = true;
}

void EditorHtmlView::endPointerGesture(const nanogui::Vector2i &local_p) {
	if (!m_mouse_down) {
		m_dragging = false;
		m_press_on_top = false;
		m_press_x = m_press_y = -1;
		return;
	}

	if (m_press_on_top && !m_dragging) {
		// Classic button: press on Top, release without drag → go to top
		// even if the pointer drifted a few pixels off the chrome.
		jumpToTop();
	} else if (!m_dragging && !m_press_on_top) {
		fireDocumentClick(local_p);
	} else if (m_doc) {
		m_doc->on_button_cancel([](const litehtml::position &) {});
	}

	m_mouse_down = false;
	m_dragging = false;
	m_press_on_top = false;
	m_press_x = m_press_y = -1;
}

bool EditorHtmlView::handlePointerMove(const nanogui::Vector2i &local_p) {
	if (!m_mouse_down || m_press_x < 0)
		return false;

	const int dx = std::abs(local_p.x() - m_press_x);
	const int dy = std::abs(local_p.y() - m_press_y);
	if (!m_dragging && (dx >= kDragThresholdPx || dy >= kDragThresholdPx)) {
		m_dragging = true;
		// Top-button press that turns into a drag becomes scroll instead.
		m_press_on_top = false;
		if (m_doc)
			m_doc->on_button_cancel([](const litehtml::position &) {});
	}
	if (m_dragging) {
		setScrollY(m_press_scroll0 - (local_p.y() - m_press_y));
		return true;
	}
	return true; // consumed press-move within click threshold
}

bool EditorHtmlView::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) {
	// editorMouseButtonEvent: false = edit-mode selection consumed the event;
	// true = run mode — handle document interaction here (not Widget default).
	if (!editorMouseButtonEvent(this, p, button, down, modifiers))
		return true;

	if (button != GLFW_MOUSE_BUTTON_LEFT)
		return true;

	// NanoGUI passes parent-relative coordinates; litehtml / Top chrome use local.
	const nanogui::Vector2i local = p - mPos;

	if (down) {
		requestFocus();
		m_mouse_down = true;
		m_dragging = false;
		m_press_x = local.x();
		m_press_y = local.y();
		m_press_scroll0 = m_scroll_y;
		m_press_on_top = hitTopButton(local);
		// Defer litehtml click until release so drag-scroll never activates links.
		if (m_doc && !m_press_on_top) {
			auto d = docCoords(local);
			if (m_doc->on_mouse_over(d.x(), d.y(), local.x(), local.y(), [](const litehtml::position &) {}))
				m_need_paint = true;
		}
	} else {
		endPointerGesture(local);
	}
	return true;
}

bool EditorHtmlView::mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button,
									  int modifiers) {
	if (!editorMouseMotionEvent(this, p, rel, button, modifiers))
		return true;

	const nanogui::Vector2i local = p - mPos;
	const bool left_down = (button & (1 << GLFW_MOUSE_BUTTON_LEFT)) != 0;
	if (m_mouse_down && left_down)
		return handlePointerMove(local);

	// Hover styles when not pressing
	if (m_doc && !m_mouse_down) {
		auto d = docCoords(local);
		if (m_doc->on_mouse_over(d.x(), d.y(), local.x(), local.y(), [](const litehtml::position &) {}))
			m_need_paint = true;
	}
	return true;
}

bool EditorHtmlView::mouseDragEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button,
									int modifiers) {
	// Nanogui drag path: p is parent-relative (screen - parent.absolutePosition).
	(void)button;
	(void)modifiers;
	const nanogui::Vector2i local = toLocal(p);
	if (!editorMouseMotionEvent(this, local, rel, button, modifiers))
		return true;
	return handlePointerMove(local);
}

bool EditorHtmlView::mouseEnterEvent(const nanogui::Vector2i &p, bool enter) {
	if (!editorMouseEnterEvent(this, p, enter))
		return true;
	if (!enter && m_doc) {
		if (m_doc->on_mouse_leave([](const litehtml::position &) {}))
			m_need_paint = true;
	}
	return true;
}

bool EditorHtmlView::scrollEvent(const nanogui::Vector2i & /*p*/, const nanogui::Vector2f &rel) {
	setScrollY(m_scroll_y - (int)(rel.y() * 40));
	return true;
}

bool EditorHtmlView::keyboardEvent(int key, int scancode, int action, int modifiers) {
	if (Widget::keyboardEvent(key, scancode, action, modifiers))
		return true;

	if (!m_doc || (action != GLFW_PRESS && action != GLFW_REPEAT))
		return false;

	const int page = std::max(40, height() - 40);
	const int line = 40;

	switch (key) {
	case GLFW_KEY_HOME:
		jumpToTop();
		return true;
	case GLFW_KEY_END:
		setScrollY(std::max(0, m_content_height - height()));
		return true;
	case GLFW_KEY_PAGE_UP:
		setScrollY(m_scroll_y - page);
		return true;
	case GLFW_KEY_PAGE_DOWN:
		setScrollY(m_scroll_y + page);
		return true;
	case GLFW_KEY_UP:
		setScrollY(m_scroll_y - line);
		return true;
	case GLFW_KEY_DOWN:
		setScrollY(m_scroll_y + line);
		return true;
	default:
		break;
	}
	return false;
}

void EditorHtmlView::getPropertyNames(std::list<std::string> &names) {
	EditorWidget::getPropertyNames(names);
	names.push_back("URL");
}

void EditorHtmlView::loadPropertyToStructureMap(std::map<std::string, std::string> &properties) {
	properties = property_map();
}

Value EditorHtmlView::getPropertyValue(const std::string &prop) {
	Value res = EditorWidget::getPropertyValue(prop);
	if (res != SymbolTable::Null)
		return res;
	if (prop == "URL")
		return Value(m_url, Value::t_string);
	return SymbolTable::Null;
}

void EditorHtmlView::setProperty(const std::string &prop, const std::string value) {
	EditorWidget::setProperty(prop, value);
	if (prop == "URL") {
		setUrl(value, true);
	} else if (prop == "Remote") {
		if (remote)
			remote->unlink(this);
		remote = EDITOR->gui()->findLinkableProperty(value);
		if (remote)
			remote->link(new LinkableText(this));
	}
}

void EditorHtmlView::loadProperties(PropertyFormHelper *properties) {
	EditorWidget::loadProperties(properties);
	properties->addVariable<std::string>(
		"URL", [&](std::string value) mutable { setUrl(value, true); }, [&]() -> std::string { return m_url; });
	properties->addGroup("Remote");
	properties->addVariable<std::string>(
		"Remote object",
		[&, this](std::string value) {
			LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(value);
			this->setRemoteName(value);
			if (remote)
				remote->unlink(this);
			remote = lp;
			if (lp)
				lp->link(new LinkableText(this));
		},
		[&]() -> std::string {
			if (remote)
				return remote->tagName();
			return getRemoteName();
		});
}
