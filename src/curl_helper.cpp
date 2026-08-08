//
//  curl_helper.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include "curl_helper.h"

#include <curl/curl.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct BufferInfo {
	size_t size = 0;
	size_t len = 0;
	char *buffer = nullptr;
};

size_t receive_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	const size_t n = size * nmemb;
	BufferInfo *bufp = static_cast<BufferInfo *>(userp);
	if (!bufp)
		return 0;
	if (bufp->len + n + 1 > bufp->size) {
		const size_t newsize = bufp->len + n + 1;
		char *newbuf = static_cast<char *>(malloc(newsize));
		if (!newbuf)
			return 0;
		if (bufp->buffer && bufp->len)
			memmove(newbuf, bufp->buffer, bufp->len);
		memmove(newbuf + bufp->len, buffer, n);
		newbuf[bufp->len + n] = 0;
		free(bufp->buffer);
		bufp->buffer = newbuf;
		bufp->size = newsize;
		bufp->len = bufp->len + n;
	} else {
		memmove(bufp->buffer + bufp->len, buffer, n);
		bufp->len += n;
		if (bufp->size > bufp->len)
			bufp->buffer[bufp->len] = 0;
	}
	return n;
}

// Header capture for ETag / Last-Modified / Content-Length (case-insensitive).
struct HeaderInfo {
	std::string etag;
	std::string last_modified;
	long content_length = -1;
};

static void trim_crlf(std::string &s) {
	while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ' || s.back() == '\t'))
		s.pop_back();
	size_t i = 0;
	while (i < s.size() && (s[i] == ' ' || s[i] == '\t'))
		++i;
	if (i)
		s = s.substr(i);
}

static bool ieq_prefix(const char *line, size_t n, const char *key) {
	const size_t klen = strlen(key);
	if (n < klen)
		return false;
	for (size_t i = 0; i < klen; ++i) {
		char a = line[i];
		char b = key[i];
		if (a >= 'A' && a <= 'Z')
			a = static_cast<char>(a - 'A' + 'a');
		if (b >= 'A' && b <= 'Z')
			b = static_cast<char>(b - 'A' + 'a');
		if (a != b)
			return false;
	}
	return true;
}

size_t receive_header(char *buffer, size_t size, size_t nitems, void *userdata) {
	const size_t n = size * nitems;
	HeaderInfo *h = static_cast<HeaderInfo *>(userdata);
	if (!h || n < 2)
		return n;
	if (ieq_prefix(buffer, n, "etag:")) {
		std::string v(buffer + 5, n - 5);
		trim_crlf(v);
		h->etag = v;
	} else if (ieq_prefix(buffer, n, "last-modified:")) {
		std::string v(buffer + 14, n - 14);
		trim_crlf(v);
		h->last_modified = v;
	} else if (ieq_prefix(buffer, n, "content-length:")) {
		std::string v(buffer + 15, n - 15);
		trim_crlf(v);
		h->content_length = strtol(v.c_str(), nullptr, 10);
	}
	return n;
}

bool ensure_curl_global() {
	// Safe to call multiple times; CURL_GLOBAL_NOTHING avoids extra SSL init if already done.
	static bool inited = false;
	if (inited)
		return true;
	const CURLcode rc = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (rc != CURLE_OK)
		return false;
	inited = true;
	return true;
}

bool write_buffer_to_file(const BufferInfo &buf, const std::string &filename) {
	if (!buf.buffer && buf.len == 0) {
		// Allow empty body (rare)
		FILE *f = fopen(filename.c_str(), "wb");
		if (!f)
			return false;
		fclose(f);
		return true;
	}
	if (!buf.buffer)
		return false;
	FILE *f = fopen(filename.c_str(), "wb");
	if (!f)
		return false;
	const size_t written = fwrite(buf.buffer, 1, buf.len, f);
	fclose(f);
	return written == buf.len;
}

struct EasySlot {
	CURL *easy = nullptr;
	struct curl_slist *headers = nullptr;
	BufferInfo body;
	HeaderInfo hdr;
	HttpFetchJob *job = nullptr;
	std::string if_none_match_line;
	std::string if_mod_since_line;
	bool active = false;
};

void release_slot(EasySlot &s) {
	if (s.headers) {
		curl_slist_free_all(s.headers);
		s.headers = nullptr;
	}
	if (s.easy) {
		curl_easy_cleanup(s.easy);
		s.easy = nullptr;
	}
	free(s.body.buffer);
	s.body.buffer = nullptr;
	s.body.size = s.body.len = 0;
	s.active = false;
	s.job = nullptr;
}

bool setup_easy(EasySlot &s, HttpFetchJob &job, long timeout_sec) {
	s.easy = curl_easy_init();
	if (!s.easy)
		return false;
	s.job = &job;
	s.body.size = s.body.len = 0;
	s.body.buffer = nullptr;
	s.hdr = HeaderInfo();

	curl_easy_setopt(s.easy, CURLOPT_URL, job.url.c_str());
	curl_easy_setopt(s.easy, CURLOPT_WRITEFUNCTION, receive_data);
	curl_easy_setopt(s.easy, CURLOPT_WRITEDATA, &s.body);
	curl_easy_setopt(s.easy, CURLOPT_HEADERFUNCTION, receive_header);
	curl_easy_setopt(s.easy, CURLOPT_HEADERDATA, &s.hdr);
	curl_easy_setopt(s.easy, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(s.easy, CURLOPT_TIMEOUT, timeout_sec);
	curl_easy_setopt(s.easy, CURLOPT_NOSIGNAL, 1L);
	// Check HTTP status ourselves so 304 is not treated as an error.
	curl_easy_setopt(s.easy, CURLOPT_FAILONERROR, 0L);

	s.headers = nullptr;
	if (!job.validators.etag.empty()) {
		s.if_none_match_line = "If-None-Match: " + job.validators.etag;
		s.headers = curl_slist_append(s.headers, s.if_none_match_line.c_str());
	}
	if (!job.validators.last_modified.empty()) {
		s.if_mod_since_line = "If-Modified-Since: " + job.validators.last_modified;
		s.headers = curl_slist_append(s.headers, s.if_mod_since_line.c_str());
	}
	if (s.headers)
		curl_easy_setopt(s.easy, CURLOPT_HTTPHEADER, s.headers);

	curl_easy_setopt(s.easy, CURLOPT_PRIVATE, reinterpret_cast<char *>(&s));
	s.active = true;
	return true;
}

void finish_slot(EasySlot &s, CURLcode result) {
	HttpFetchJob *job = s.job;
	if (!job) {
		release_slot(s);
		return;
	}

	HttpFetchResult &out = job->result;
	out = HttpFetchResult();
	out.etag = s.hdr.etag;
	out.last_modified = s.hdr.last_modified;
	out.content_length = s.hdr.content_length;

	if (result != CURLE_OK) {
		out.status = HttpFetchStatus::Failed;
		out.error = curl_easy_strerror(result);
		std::cerr << "Error " << (int)result << " from curl when retrieving " << job->url << "\n";
		release_slot(s);
		return;
	}

	long code = 0;
	curl_easy_getinfo(s.easy, CURLINFO_RESPONSE_CODE, &code);
	out.http_code = code;

	if (code == 304) {
		out.status = HttpFetchStatus::NotModified;
		// Prefer validators already known; fill from response headers if present.
		if (out.etag.empty())
			out.etag = job->validators.etag;
		if (out.last_modified.empty())
			out.last_modified = job->validators.last_modified;
		release_slot(s);
		return;
	}

	if (code < 200 || code >= 300) {
		out.status = HttpFetchStatus::Failed;
		out.error = "HTTP " + std::to_string(code);
		std::cerr << "HTMLVIEW/curl: HTTP " << code << " for " << job->url << "\n";
		release_slot(s);
		return;
	}

	if (!write_buffer_to_file(s.body, job->filename)) {
		out.status = HttpFetchStatus::Failed;
		out.error = "write failed";
		std::cerr << "error: failed writing " << job->filename << "\n";
		release_slot(s);
		return;
	}

	if (out.content_length < 0)
		out.content_length = static_cast<long>(s.body.len);
	out.status = HttpFetchStatus::Ok;
	release_slot(s);
}

} // namespace

bool get_file(const std::string url_s, const std::string filename) {
	const HttpFetchResult r = fetch_url_to_file(url_s, filename, nullptr, 30);
	return r.status == HttpFetchStatus::Ok;
}

HttpFetchResult fetch_url_to_file(const std::string &url, const std::string &filename,
								  const HttpCacheValidators *validators, long timeout_sec) {
	HttpFetchJob job;
	job.url = url;
	job.filename = filename;
	if (validators)
		job.validators = *validators;
	std::vector<HttpFetchJob> jobs(1, job);
	fetch_urls_to_files(jobs, 1, timeout_sec);
	return jobs[0].result;
}

void fetch_urls_to_files(std::vector<HttpFetchJob> &jobs, int max_parallel, long timeout_sec) {
	if (jobs.empty())
		return;
	if (max_parallel < 1)
		max_parallel = 1;
	if (!ensure_curl_global()) {
		for (size_t i = 0; i < jobs.size(); ++i) {
			jobs[i].result.status = HttpFetchStatus::Failed;
			jobs[i].result.error = "curl_global_init failed";
		}
		return;
	}

	CURLM *multi = curl_multi_init();
	if (!multi) {
		for (size_t i = 0; i < jobs.size(); ++i) {
			jobs[i].result.status = HttpFetchStatus::Failed;
			jobs[i].result.error = "curl_multi_init failed";
		}
		return;
	}

	// Bound parallel slots
	const int slots_n = max_parallel < (int)jobs.size() ? max_parallel : (int)jobs.size();
	std::vector<EasySlot> slots(static_cast<size_t>(slots_n));
	size_t next_job = 0;
	int still_running = 0;

	auto start_next = [&]() -> bool {
		while (next_job < jobs.size()) {
			EasySlot *open = nullptr;
			for (size_t i = 0; i < slots.size(); ++i) {
				if (!slots[i].active) {
					open = &slots[i];
					break;
				}
			}
			if (!open)
				return false;
			HttpFetchJob &job = jobs[next_job++];
			if (!setup_easy(*open, job, timeout_sec)) {
				job.result.status = HttpFetchStatus::Failed;
				job.result.error = "curl_easy_init failed";
				continue;
			}
			const CURLMcode mc = curl_multi_add_handle(multi, open->easy);
			if (mc != CURLM_OK) {
				job.result.status = HttpFetchStatus::Failed;
				job.result.error = "curl_multi_add_handle failed";
				release_slot(*open);
				continue;
			}
			return true;
		}
		return false;
	};

	// Prime
	for (int i = 0; i < slots_n; ++i)
		start_next();

	curl_multi_perform(multi, &still_running);

	while (still_running > 0 || next_job < jobs.size()) {
		int numfds = 0;
		curl_multi_wait(multi, nullptr, 0, 1000, &numfds);
		curl_multi_perform(multi, &still_running);

		CURLMsg *msg = nullptr;
		int msgs_left = 0;
		while ((msg = curl_multi_info_read(multi, &msgs_left)) != nullptr) {
			if (msg->msg != CURLMSG_DONE)
				continue;
			CURL *easy = msg->easy_handle;
			CURLcode result = msg->data.result;
			char *priv = nullptr;
			curl_easy_getinfo(easy, CURLINFO_PRIVATE, &priv);
			EasySlot *slot = reinterpret_cast<EasySlot *>(priv);
			curl_multi_remove_handle(multi, easy);
			if (slot)
				finish_slot(*slot, result);
			start_next();
		}

		if (still_running == 0 && next_job >= jobs.size())
			break;
		if (still_running == 0 && next_job < jobs.size()) {
			bool started = false;
			while (start_next())
				started = true;
			if (!started)
				break;
			curl_multi_perform(multi, &still_running);
		}
	}

	for (size_t i = 0; i < slots.size(); ++i) {
		if (slots[i].active && slots[i].easy) {
			curl_multi_remove_handle(multi, slots[i].easy);
			if (slots[i].job) {
				slots[i].job->result.status = HttpFetchStatus::Failed;
				if (slots[i].job->result.error.empty())
					slots[i].job->result.error = "aborted";
			}
			release_slot(slots[i]);
		}
	}
	curl_multi_cleanup(multi);
}
