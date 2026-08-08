//
//  htmlview_container.cpp
//  Project: humid
//

#include "htmlview_container.h"
#include "curl_helper.h"

#include <boost/filesystem.hpp>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = boost::filesystem;

namespace {
// Avoid unbounded growth if many manuals are opened in one humid session.
const size_t kMaxCachedImages = 128;
}

HtmlViewContainer::HtmlViewContainer() {
	// Unique directory per container so destructor can remove only our files.
	try {
		fs::path base = fs::temp_directory_path() / "humid-htmlview-cache";
		fs::create_directories(base);
		m_cache_dir = (base / fs::unique_path("sess-%%%%-%%%%-%%%%-%%%%")).string();
		fs::create_directories(m_cache_dir);
		m_owns_cache_dir = true;
	} catch (...) {
		m_cache_dir = (fs::temp_directory_path() / "humid-htmlview-cache-fallback").string();
		try {
			fs::create_directories(m_cache_dir);
			m_owns_cache_dir = true;
		} catch (...) {
			m_owns_cache_dir = false;
		}
	}
}

HtmlViewContainer::~HtmlViewContainer() {
	clearImageSurfaces();
	clearDiskCache();
}

void HtmlViewContainer::clearImageSurfaces() {
	for (auto &kv : m_images) {
		if (kv.second)
			cairo_surface_destroy(kv.second);
	}
	m_images.clear();
}

void HtmlViewContainer::clearDiskCache() {
	m_url_to_local.clear();
	m_downloaded_paths.clear();
	if (!m_owns_cache_dir || m_cache_dir.empty())
		return;
	try {
		if (fs::exists(m_cache_dir) && fs::is_directory(m_cache_dir))
			fs::remove_all(m_cache_dir);
	} catch (const std::exception &e) {
		std::cerr << "HTMLVIEW: cache cleanup failed: " << e.what() << "\n";
	}
	m_owns_cache_dir = false;
	m_cache_dir.clear();
}

void HtmlViewContainer::setBaseUrl(const std::string &base_url) {
	m_base_url = base_url;
	// Drop decoded bitmaps when document base changes; keep on-disk downloads
	// for the same session so revisits do not re-fetch.
	clearImageSurfaces();
}

void HtmlViewContainer::setViewport(int width, int height) {
	m_width = width > 1 ? width : 1;
	m_height = height > 1 ? height : 1;
}

void HtmlViewContainer::set_base_url(const char *base_url) {
	if (base_url && *base_url)
		setBaseUrl(base_url);
}

void HtmlViewContainer::set_caption(const char *) {}
void HtmlViewContainer::on_mouse_event(const litehtml::element::ptr &, litehtml::mouse_event) {}
void HtmlViewContainer::set_cursor(const char *) {}

void HtmlViewContainer::on_anchor_click(const char *url, const litehtml::element::ptr &) {
	if (!url)
		return;
	m_last_anchor = url;
	if (m_anchor_cb)
		m_anchor_cb(m_last_anchor);
}

void HtmlViewContainer::get_viewport(litehtml::position &viewport) const {
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = m_width;
	viewport.height = m_height;
}

double HtmlViewContainer::get_screen_dpi() const { return m_dpi; }
int HtmlViewContainer::get_screen_width() const { return m_width; }
int HtmlViewContainer::get_screen_height() const { return m_height; }

std::string HtmlViewContainer::resolveUrl(const std::string &url, const std::string &base) const {
	if (url.empty())
		return base;
	if (url.find("http://") == 0 || url.find("https://") == 0 || url.find("file:") == 0)
		return url;
	if (!url.empty() && url[0] == '/') {
		auto scheme = base.find("://");
		if (scheme != std::string::npos) {
			auto host_end = base.find('/', scheme + 3);
			if (host_end == std::string::npos)
				return base + url;
			return base.substr(0, host_end) + url;
		}
		return url;
	}
	std::string b = base.empty() ? m_base_url : base;
	if (b.empty())
		return url;
	auto hash = b.find('#');
	if (hash != std::string::npos)
		b = b.substr(0, hash);
	auto q = b.find('?');
	if (q != std::string::npos)
		b = b.substr(0, q);
	auto slash = b.find_last_of('/');
	if (slash == std::string::npos)
		return url;
	return b.substr(0, slash + 1) + url;
}

void HtmlViewContainer::make_url(const char *url, const char *basepath, std::string &out) {
	std::string u = url ? url : "";
	std::string b = basepath && *basepath ? basepath : m_base_url;
	out = resolveUrl(u, b);
}

std::string HtmlViewContainer::cachePathForUrl(const std::string &url) const {
	std::string key = url;
	for (char &c : key) {
		if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '-' || c == '_'))
			c = '_';
	}
	if (key.size() > 180)
		key = key.substr(key.size() - 180);
	return (fs::path(m_cache_dir) / key).string();
}

bool HtmlViewContainer::ensureLocalFile(const std::string &url, std::string &local_path) {
	auto it = m_url_to_local.find(url);
	if (it != m_url_to_local.end() && fs::exists(it->second)) {
		local_path = it->second;
		return true;
	}

	if (url.find("file://") == 0) {
		local_path = url.substr(7);
		if (fs::exists(local_path)) {
			m_url_to_local[url] = local_path;
			return true;
		}
		return false;
	}
	if (url.find("http://") != 0 && url.find("https://") != 0) {
		if (fs::exists(url)) {
			local_path = url;
			m_url_to_local[url] = local_path;
			return true;
		}
		return false;
	}

	if (m_cache_dir.empty())
		return false;

	local_path = cachePathForUrl(url);
	if (!fs::exists(local_path)) {
		if (!get_file(url, local_path)) {
			std::cerr << "HTMLVIEW: failed to fetch " << url << "\n";
			// Remove partial file if any
			try {
				if (fs::exists(local_path))
					fs::remove(local_path);
			} catch (...) {
			}
			return false;
		}
		m_downloaded_paths.insert(local_path);
	}
	m_url_to_local[url] = local_path;
	return true;
}

cairo_surface_t *HtmlViewContainer::loadSurface(const std::string &local_path) {
	cairo_surface_t *surf = cairo_image_surface_create_from_png(local_path.c_str());
	if (surf && cairo_surface_status(surf) == CAIRO_STATUS_SUCCESS)
		return surf;
	if (surf)
		cairo_surface_destroy(surf);
	return nullptr;
}

void HtmlViewContainer::load_image(const char *src, const char *baseurl, bool) {
	if (!src)
		return;
	std::string full;
	make_url(src, baseurl, full);
	if (m_images.count(full))
		return;
	std::string local;
	if (!ensureLocalFile(full, local))
		return;
	if (m_images.size() >= kMaxCachedImages) {
		// Evict oldest by map order (stable enough for manuals)
		auto it = m_images.begin();
		if (it->second)
			cairo_surface_destroy(it->second);
		m_images.erase(it);
	}
	cairo_surface_t *surf = loadSurface(local);
	if (surf)
		m_images[full] = surf;
}

cairo_surface_t *HtmlViewContainer::get_image(const std::string &url) {
	// litehtml container_cairo always cairo_surface_destroy()s the return value.
	auto it = m_images.find(url);
	if (it != m_images.end() && it->second)
		return cairo_surface_reference(it->second);

	std::string local;
	if (!ensureLocalFile(url, local))
		return nullptr;
	cairo_surface_t *surf = loadSurface(local);
	if (!surf)
		return nullptr;

	if (m_images.size() >= kMaxCachedImages) {
		auto evict = m_images.begin();
		if (evict != m_images.end()) {
			if (evict->second)
				cairo_surface_destroy(evict->second);
			m_images.erase(evict);
		}
	}
	m_images[url] = surf;
	return cairo_surface_reference(surf);
}

void HtmlViewContainer::import_css(std::string &text, const std::string &url, std::string &baseurl) {
	std::string full = resolveUrl(url, baseurl.empty() ? m_base_url : baseurl);
	std::string local;
	if (!ensureLocalFile(full, local)) {
		text.clear();
		return;
	}
	std::ifstream in(local.c_str(), std::ios::binary);
	if (!in) {
		text.clear();
		return;
	}
	std::ostringstream ss;
	ss << in.rdbuf();
	text = ss.str();
	baseurl = full;
}
