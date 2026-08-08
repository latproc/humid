//
//  htmlview_container.cpp
//  Project: humid
//

#include "htmlview_container.h"
#include "curl_helper.h"

#include <boost/filesystem.hpp>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdint>

namespace fs = boost::filesystem;

namespace {
// Avoid unbounded growth if many manuals are opened in one humid session.
const size_t kMaxCachedImages = 128;
const int kDefaultParallel = 8;

// Prefer a real disk cache. Never use /tmp or /dev/shm.
// Order: HUMID_HTMLVIEW_CACHE → XDG_CACHE_HOME → $HOME/.cache → /var/cache/humid.
std::string pickCacheRoot() {
	if (const char *env = std::getenv("HUMID_HTMLVIEW_CACHE")) {
		if (env[0])
			return std::string(env);
	}
	if (const char *xdg = std::getenv("XDG_CACHE_HOME")) {
		if (xdg[0])
			return (fs::path(xdg) / "humid" / "htmlview").string();
	}
	if (const char *home = std::getenv("HOME")) {
		if (home[0])
			return (fs::path(home) / ".cache" / "humid" / "htmlview").string();
	}
	return "/var/cache/humid/htmlview";
}

bool pathIsForbiddenVolatile(const std::string &p) {
	// Refuse to place the persistent cache under tmp or shm even if env points there.
	try {
		fs::path path(p);
		std::string s = path.lexically_normal().string();
		if (s == "/tmp" || s.find("/tmp/") == 0)
			return true;
		if (s == "/var/tmp" || s.find("/var/tmp/") == 0)
			return true;
		if (s == "/dev/shm" || s.find("/dev/shm/") == 0)
			return true;
		if (s.find("/private/tmp/") == 0 || s == "/private/tmp") // macOS
			return true;
	} catch (...) {
	}
	return false;
}

// FNV-1a 64-bit → hex (stable, no crypto dependency).
std::string fnv1aHex(const std::string &s) {
	uint64_t h = 14695981039346656037ull;
	for (unsigned char c : s) {
		h ^= c;
		h *= 1099511628211ull;
	}
	char buf[17];
	std::snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(h));
	return std::string(buf);
}

void extractQuotedAttrs(const std::string &html, const char *attr, std::vector<std::string> &out) {
	// attr="..." or attr='...' (case-insensitive attr name)
	const size_t alen = std::strlen(attr);
	for (size_t i = 0; i + alen + 3 < html.size(); ++i) {
		bool match = true;
		for (size_t k = 0; k < alen; ++k) {
			char a = html[i + k];
			char b = attr[k];
			if (a >= 'A' && a <= 'Z')
				a = static_cast<char>(a - 'A' + 'a');
			if (a != b) {
				match = false;
				break;
			}
		}
		if (!match)
			continue;
		size_t j = i + alen;
		while (j < html.size() && (html[j] == ' ' || html[j] == '\t' || html[j] == '\n' || html[j] == '\r'))
			++j;
		if (j >= html.size() || html[j] != '=')
			continue;
		++j;
		while (j < html.size() && (html[j] == ' ' || html[j] == '\t'))
			++j;
		if (j >= html.size())
			continue;
		char q = html[j];
		if (q != '"' && q != '\'')
			continue;
		++j;
		const size_t start = j;
		while (j < html.size() && html[j] != q)
			++j;
		if (j > start)
			out.push_back(html.substr(start, j - start));
		i = j;
	}
}

bool isHttpUrl(const std::string &url) {
	return url.find("http://") == 0 || url.find("https://") == 0;
}
} // namespace

std::string HtmlViewContainer::sharedCacheRoot() {
	static std::string root;
	static bool ready = false;
	if (ready)
		return root;

	std::string chosen = pickCacheRoot();
	if (pathIsForbiddenVolatile(chosen)) {
		std::cerr << "HTMLVIEW: refusing volatile cache path '" << chosen
				  << "', using /var/cache/humid/htmlview\n";
		chosen = "/var/cache/humid/htmlview";
	}
	try {
		fs::create_directories(chosen);
		// Verify we can write
		fs::path probe = fs::path(chosen) / ".write-test";
		{
			std::ofstream o(probe.string().c_str());
			o << "ok";
		}
		fs::remove(probe);
		root = chosen;
	} catch (const std::exception &e) {
		std::cerr << "HTMLVIEW: cannot use cache dir '" << chosen << "': " << e.what() << "\n";
		// Last resort still on real disk under HOME if possible
		if (const char *home = std::getenv("HOME")) {
			try {
				root = (fs::path(home) / ".cache" / "humid" / "htmlview").string();
				if (!pathIsForbiddenVolatile(root)) {
					fs::create_directories(root);
				} else {
					root.clear();
				}
			} catch (...) {
				root.clear();
			}
		}
		if (root.empty())
			std::cerr << "HTMLVIEW: persistent cache unavailable; assets will not be retained\n";
	}
	ready = true;
	if (!root.empty())
		std::cerr << "HTMLVIEW: disk cache at " << root << "\n";
	return root;
}

HtmlViewContainer::HtmlViewContainer() {
	m_cache_dir = sharedCacheRoot();
}

HtmlViewContainer::~HtmlViewContainer() {
	clearImageSurfaces();
	// Persistent disk cache is retained across sessions.
}

void HtmlViewContainer::clearImageSurfaces() {
	for (auto &kv : m_images) {
		if (kv.second)
			cairo_surface_destroy(kv.second);
	}
	m_images.clear();
}

void HtmlViewContainer::clearMemoryMaps() {
	m_url_to_local.clear();
}

void HtmlViewContainer::setBaseUrl(const std::string &base_url) {
	m_base_url = base_url;
	// Drop decoded bitmaps when document base changes; disk cache stays.
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

std::string HtmlViewContainer::resolveUrlStatic(const std::string &url, const std::string &base) {
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
	std::string b = base;
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

std::string HtmlViewContainer::resolveUrl(const std::string &url, const std::string &base) const {
	const std::string &b = base.empty() ? m_base_url : base;
	return resolveUrlStatic(url, b);
}

void HtmlViewContainer::make_url(const char *url, const char *basepath, std::string &out) {
	std::string u = url ? url : "";
	std::string b = basepath && *basepath ? basepath : m_base_url;
	out = resolveUrl(u, b);
}

std::string HtmlViewContainer::urlCacheKey(const std::string &url) {
	return fnv1aHex(url);
}

std::string HtmlViewContainer::bodyPathForUrl(const std::string &url) const {
	return (fs::path(m_cache_dir) / (urlCacheKey(url) + ".body")).string();
}

std::string HtmlViewContainer::metaPathForUrl(const std::string &url) const {
	return (fs::path(m_cache_dir) / (urlCacheKey(url) + ".meta")).string();
}

bool HtmlViewContainer::readMeta(const std::string &meta_path, CacheMeta &out) const {
	std::ifstream in(meta_path.c_str());
	if (!in)
		return false;
	CacheMeta m;
	std::string line;
	while (std::getline(in, line)) {
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		const auto eq = line.find('=');
		if (eq == std::string::npos)
			continue;
		const std::string key = line.substr(0, eq);
		const std::string val = line.substr(eq + 1);
		if (key == "url")
			m.url = val;
		else if (key == "etag")
			m.etag = val;
		else if (key == "last_modified")
			m.last_modified = val;
		else if (key == "content_length")
			m.content_length = std::strtol(val.c_str(), nullptr, 10);
	}
	if (m.url.empty() && m.etag.empty() && m.last_modified.empty())
		return false;
	out = m;
	return true;
}

bool HtmlViewContainer::writeMeta(const std::string &meta_path, const CacheMeta &meta) const {
	// Atomic-ish: write temp then rename within same directory.
	const std::string tmp = meta_path + ".tmp";
	{
		std::ofstream o(tmp.c_str(), std::ios::trunc);
		if (!o)
			return false;
		o << "url=" << meta.url << "\n";
		o << "etag=" << meta.etag << "\n";
		o << "last_modified=" << meta.last_modified << "\n";
		o << "content_length=" << meta.content_length << "\n";
		if (!o)
			return false;
	}
	try {
		fs::rename(tmp, meta_path);
	} catch (...) {
		try {
			fs::remove(meta_path);
			fs::rename(tmp, meta_path);
		} catch (...) {
			try {
				fs::remove(tmp);
			} catch (...) {
			}
			return false;
		}
	}
	return true;
}

bool HtmlViewContainer::bodyLooksComplete(const std::string &body_path, const CacheMeta &meta) const {
	try {
		if (!fs::exists(body_path) || !fs::is_regular_file(body_path))
			return false;
		const uintmax_t sz = fs::file_size(body_path);
		if (sz == 0)
			return false;
		// If server gave Content-Length when cached, require match.
		if (meta.content_length > 0 && static_cast<long>(sz) != meta.content_length)
			return false;
		return true;
	} catch (...) {
		return false;
	}
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
	if (!isHttpUrl(url)) {
		if (fs::exists(url)) {
			local_path = url;
			m_url_to_local[url] = local_path;
			return true;
		}
		return false;
	}

	if (m_cache_dir.empty()) {
		// No persistent cache dir — download to a unique name under HOME if possible, else fail.
		std::cerr << "HTMLVIEW: no disk cache; cannot fetch " << url << "\n";
		return false;
	}

	const std::string body = bodyPathForUrl(url);
	const std::string meta_path = metaPathForUrl(url);
	CacheMeta meta;
	const bool have_meta = readMeta(meta_path, meta) && (meta.url.empty() || meta.url == url);
	const bool have_body = have_meta && bodyLooksComplete(body, meta);

	HttpCacheValidators validators;
	if (have_body && meta.has_validators()) {
		validators.etag = meta.etag;
		validators.last_modified = meta.last_modified;
	}

	// Unvalidated cache (no ETag/Last-Modified): do not trust across revalidation —
	// always full GET into body path.
	const HttpCacheValidators *vptr =
		(have_body && meta.has_validators()) ? &validators : nullptr;

	// Write downloads to a temp body then rename so readers never see partial files.
	const std::string body_tmp = body + ".tmp";
	try {
		if (fs::exists(body_tmp))
			fs::remove(body_tmp);
	} catch (...) {
	}

	HttpFetchResult r = fetch_url_to_file(url, body_tmp, vptr, 30);

	if (r.status == HttpFetchStatus::NotModified) {
		if (!have_body) {
			std::cerr << "HTMLVIEW: 304 but missing body for " << url << "\n";
			return false;
		}
		// Refresh meta timestamps if server sent new ones
		if (!r.etag.empty())
			meta.etag = r.etag;
		if (!r.last_modified.empty())
			meta.last_modified = r.last_modified;
		meta.url = url;
		if (meta.has_validators())
			writeMeta(meta_path, meta);
		local_path = body;
		m_url_to_local[url] = local_path;
		try {
			fs::remove(body_tmp);
		} catch (...) {
		}
		return true;
	}

	if (r.status != HttpFetchStatus::Ok) {
		// Network failure: fall back to previously validated body if present.
		if (have_body && meta.has_validators()) {
			std::cerr << "HTMLVIEW: fetch failed (" << r.error << "), using validated cache for "
					  << url << "\n";
			local_path = body;
			m_url_to_local[url] = local_path;
			return true;
		}
		try {
			fs::remove(body_tmp);
		} catch (...) {
		}
		return false;
	}

	// New body on disk as .tmp — promote to final path
	try {
		if (fs::exists(body))
			fs::remove(body);
		fs::rename(body_tmp, body);
	} catch (const std::exception &e) {
		std::cerr << "HTMLVIEW: rename cache body failed: " << e.what() << "\n";
		try {
			fs::remove(body_tmp);
		} catch (...) {
		}
		return false;
	}

	meta.url = url;
	meta.etag = r.etag;
	meta.last_modified = r.last_modified;
	meta.content_length = r.content_length;
	if (meta.content_length < 0) {
		try {
			meta.content_length = static_cast<long>(fs::file_size(body));
		} catch (...) {
		}
	}

	// Only persist meta when the origin gave validators (user requirement).
	if (meta.has_validators()) {
		writeMeta(meta_path, meta);
	} else {
		// Unvalidated: keep body for this process via m_url_to_local, remove meta if any.
		try {
			if (fs::exists(meta_path))
				fs::remove(meta_path);
		} catch (...) {
		}
		std::cerr << "HTMLVIEW: no ETag/Last-Modified for " << url
				  << "; not treating as durable cache entry\n";
	}

	local_path = body;
	m_url_to_local[url] = local_path;
	return true;
}

void HtmlViewContainer::prefetchUrls(const std::vector<std::string> &urls, int max_parallel) {
	if (urls.empty() || m_cache_dir.empty())
		return;
	if (max_parallel < 1)
		max_parallel = kDefaultParallel;

	std::vector<HttpFetchJob> jobs;
	jobs.reserve(urls.size());
	// Track which job index maps to which url for meta updates
	std::vector<std::string> job_urls;
	std::vector<CacheMeta> prior_meta;
	std::vector<bool> had_validated_body;

	for (const std::string &url : urls) {
		if (!isHttpUrl(url))
			continue;
		if (m_url_to_local.count(url))
			continue;

		const std::string body = bodyPathForUrl(url);
		const std::string meta_path = metaPathForUrl(url);
		CacheMeta meta;
		const bool have_meta = readMeta(meta_path, meta) && (meta.url.empty() || meta.url == url);
		const bool have_body = have_meta && bodyLooksComplete(body, meta);

		HttpFetchJob job;
		job.url = url;
		job.filename = body + ".tmp";
		if (have_body && meta.has_validators()) {
			job.validators.etag = meta.etag;
			job.validators.last_modified = meta.last_modified;
		}
		try {
			if (fs::exists(job.filename))
				fs::remove(job.filename);
		} catch (...) {
		}
		jobs.push_back(job);
		job_urls.push_back(url);
		prior_meta.push_back(meta);
		had_validated_body.push_back(have_body && meta.has_validators());
	}

	if (jobs.empty())
		return;

	fetch_urls_to_files(jobs, max_parallel, 30);

	for (size_t i = 0; i < jobs.size(); ++i) {
		const std::string &url = job_urls[i];
		const std::string body = bodyPathForUrl(url);
		const std::string meta_path = metaPathForUrl(url);
		const std::string body_tmp = jobs[i].filename;
		HttpFetchResult &r = jobs[i].result;
		CacheMeta meta = prior_meta[i];

		if (r.status == HttpFetchStatus::NotModified) {
			if (had_validated_body[i]) {
				if (!r.etag.empty())
					meta.etag = r.etag;
				if (!r.last_modified.empty())
					meta.last_modified = r.last_modified;
				meta.url = url;
				if (meta.has_validators())
					writeMeta(meta_path, meta);
				m_url_to_local[url] = body;
			}
			try {
				fs::remove(body_tmp);
			} catch (...) {
			}
			continue;
		}

		if (r.status != HttpFetchStatus::Ok) {
			if (had_validated_body[i]) {
				std::cerr << "HTMLVIEW: prefetch failed (" << r.error << "), using cache for " << url
						  << "\n";
				m_url_to_local[url] = body;
			} else {
				std::cerr << "HTMLVIEW: prefetch failed for " << url << ": " << r.error << "\n";
			}
			try {
				fs::remove(body_tmp);
			} catch (...) {
			}
			continue;
		}

		try {
			if (fs::exists(body))
				fs::remove(body);
			fs::rename(body_tmp, body);
		} catch (const std::exception &e) {
			std::cerr << "HTMLVIEW: prefetch rename failed: " << e.what() << "\n";
			try {
				fs::remove(body_tmp);
			} catch (...) {
			}
			continue;
		}

		meta.url = url;
		meta.etag = r.etag;
		meta.last_modified = r.last_modified;
		meta.content_length = r.content_length;
		if (meta.content_length < 0) {
			try {
				meta.content_length = static_cast<long>(fs::file_size(body));
			} catch (...) {
			}
		}
		if (meta.has_validators()) {
			writeMeta(meta_path, meta);
		} else {
			try {
				if (fs::exists(meta_path))
					fs::remove(meta_path);
			} catch (...) {
			}
		}
		m_url_to_local[url] = body;
	}
}

std::vector<std::string> HtmlViewContainer::collectAssetUrls(const std::string &html,
															 const std::string &base_url) {
	std::vector<std::string> raw;
	extractQuotedAttrs(html, "src", raw);
	// stylesheets: href= on link tags — collect all href and filter .css
	std::vector<std::string> hrefs;
	extractQuotedAttrs(html, "href", hrefs);
	for (const std::string &h : hrefs) {
		std::string lower = h;
		for (char &c : lower) {
			if (c >= 'A' && c <= 'Z')
				c = static_cast<char>(c - 'A' + 'a');
		}
		if (lower.size() >= 4 && lower.find(".css") != std::string::npos)
			raw.push_back(h);
	}

	std::vector<std::string> out;
	out.reserve(raw.size());
	std::map<std::string, bool> seen;
	for (const std::string &r : raw) {
		if (r.empty() || r[0] == '#')
			continue;
		if (r.find("data:") == 0)
			continue;
		std::string full = resolveUrlStatic(r, base_url);
		if (!isHttpUrl(full))
			continue;
		if (seen.count(full))
			continue;
		seen[full] = true;
		out.push_back(full);
	}
	return out;
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
