//
//  htmlview_container.h
//  Project: humid
//
//  litehtml document_container backed by Cairo/Pango with curl asset cache.
//  Ownership notes:
//  - m_images holds one cairo_surface_t* per URL (refcount 1).
//  - get_image() returns cairo_surface_reference(); caller (litehtml cairo
//    container) must cairo_surface_destroy() that reference.
//  - Disk cache is persistent under a real disk path (not /tmp, not /dev/shm).
//    Entries are kept only when the origin provides validators (ETag and/or
//    Last-Modified); revalidation uses conditional GET (304).
//

#ifndef __htmlview_container_h__
#define __htmlview_container_h__

#include "container_cairo_pango.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

class HtmlViewContainer : public container_cairo_pango {
public:
	using AnchorClickCallback = std::function<void(const std::string &href)>;

	HtmlViewContainer();
	~HtmlViewContainer() override;

	HtmlViewContainer(const HtmlViewContainer &) = delete;
	HtmlViewContainer &operator=(const HtmlViewContainer &) = delete;

	void setBaseUrl(const std::string &base_url);
	const std::string &baseUrl() const { return m_base_url; }

	void setViewport(int width, int height);
	void setAnchorClickCallback(AnchorClickCallback cb) { m_anchor_cb = std::move(cb); }

	void clearImageSurfaces();
	// Clears in-memory URL maps only. Does not delete persistent disk cache.
	void clearMemoryMaps();

	// Fetch (or revalidate) a single URL into the persistent cache.
	// On success, local_path is a durable on-disk path; html text can be read from it.
	bool ensureLocalFile(const std::string &url, std::string &local_path);

	// Parallel fetch/revalidate of many URLs (CSS, images, …). Safe before parse.
	void prefetchUrls(const std::vector<std::string> &urls, int max_parallel = 8);

	// Scan HTML for stylesheet hrefs and img src; resolve against base_url; return absolute URLs.
	static std::vector<std::string> collectAssetUrls(const std::string &html,
													 const std::string &base_url);

	void load_image(const char *src, const char *baseurl, bool redraw_on_ready) override;
	void import_css(std::string &text, const std::string &url, std::string &baseurl) override;
	void set_base_url(const char *base_url) override;
	void set_caption(const char *caption) override;
	void on_anchor_click(const char *url, const litehtml::element::ptr &el) override;
	void on_mouse_event(const litehtml::element::ptr &el, litehtml::mouse_event event) override;
	void set_cursor(const char *cursor) override;
	void get_viewport(litehtml::position &viewport) const override;
	void make_url(const char *url, const char *basepath, std::string &out) override;
	cairo_surface_t *get_image(const std::string &url) override;
	double get_screen_dpi() const override;
	int get_screen_width() const override;
	int get_screen_height() const override;

	const std::string &lastAnchor() const { return m_last_anchor; }
	const std::string &cacheDir() const { return m_cache_dir; }

private:
	struct CacheMeta {
		std::string url;
		std::string etag;
		std::string last_modified;
		long content_length = -1;
		bool has_validators() const { return !etag.empty() || !last_modified.empty(); }
	};

	std::string resolveUrl(const std::string &url, const std::string &base) const;
	static std::string resolveUrlStatic(const std::string &url, const std::string &base);
	static std::string urlCacheKey(const std::string &url);
	std::string bodyPathForUrl(const std::string &url) const;
	std::string metaPathForUrl(const std::string &url) const;
	bool readMeta(const std::string &meta_path, CacheMeta &out) const;
	bool writeMeta(const std::string &meta_path, const CacheMeta &meta) const;
	bool bodyLooksComplete(const std::string &body_path, const CacheMeta &meta) const;
	cairo_surface_t *loadSurface(const std::string &local_path);
	// Shared disk cache root for this process (created on first use).
	static std::string sharedCacheRoot();

	std::string m_base_url;
	std::string m_cache_dir;
	int m_width = 800;
	int m_height = 600;
	double m_dpi = 96.0;
	std::string m_last_anchor;
	AnchorClickCallback m_anchor_cb;
	std::map<std::string, cairo_surface_t *> m_images;
	std::map<std::string, std::string> m_url_to_local;
};

#endif
