//
//  curl_helper.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __curl_helper_h__
#define __curl_helper_h__

#include <ostream>
#include <string>
#include <vector>

// Simple download (always writes body on 2xx). Used by non-HTMLVIEW paths.
bool get_file(const std::string url, const std::string filename);

// Optional validators for conditional GET (If-None-Match / If-Modified-Since).
struct HttpCacheValidators {
	std::string etag;
	std::string last_modified;
};

enum class HttpFetchStatus {
	Ok,          // new body written to path
	NotModified, // 304; existing path left unchanged
	Failed
};

struct HttpFetchResult {
	HttpFetchStatus status = HttpFetchStatus::Failed;
	long http_code = 0;
	std::string etag;
	std::string last_modified;
	long content_length = -1;
	std::string error;
};

// Single URL → file. If validators is non-null and has etag and/or last_modified,
// sends conditional headers; 304 → NotModified (file not rewritten).
HttpFetchResult fetch_url_to_file(const std::string &url, const std::string &filename,
								  const HttpCacheValidators *validators = nullptr,
								  long timeout_sec = 30);

struct HttpFetchJob {
	std::string url;
	std::string filename;
	HttpCacheValidators validators; // empty → unconditional GET
	HttpFetchResult result;
};

// Parallel downloads via curl multi (single-threaded event loop).
// max_parallel caps in-flight transfers (default 8).
void fetch_urls_to_files(std::vector<HttpFetchJob> &jobs, int max_parallel = 8,
						 long timeout_sec = 30);

#endif
