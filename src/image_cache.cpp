//
//  image_cache.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include "image_cache.h"

#include <boost/filesystem.hpp>
#include <sys/statvfs.h>

#include <algorithm>
#include <cctype>
#include <vector>

namespace fs = boost::filesystem;

const char kHttpImageCacheDir[] = "cache";

namespace {

bool imageExtension(const std::string &name) {
	const std::string::size_type dot = name.rfind('.');
	if (dot == std::string::npos || dot == 0 || dot + 1 >= name.size())
		return false;
	std::string ext = name.substr(dot + 1);
	for (char &c : ext) {
		if (c >= 'A' && c <= 'Z')
			c = static_cast<char>(c - 'A' + 'a');
	}
	return ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "gif" ||
		ext == "bmp" || ext == "tga" || ext == "ppm" || ext == "pgm" || ext == "pnm";
}

// True for "cache/page001.png": one path component, no "..", image suffix.
bool isManagedCacheFile(const fs::path &cache_dir, const fs::path &file) {
	if (file.empty())
		return false;
	for (fs::path::iterator it = file.begin(); it != file.end(); ++it) {
		if (*it == "..")
			return false;
	}
	const std::string base = file.filename().string();
	if (base.empty() || base[0] == '.' || !imageExtension(base))
		return false;
	fs::path dir = fs::absolute(cache_dir);
	fs::path candidate = fs::absolute(file);
	if (dir.filename() == ".")
		dir = dir.parent_path();
	return candidate.parent_path() == dir;
}

bool removeRegularFile(const fs::path &path) {
	boost::system::error_code ec;
	const fs::file_status st = fs::symlink_status(path, ec);
	if (ec || !fs::is_regular_file(st))
		return false;
	fs::remove(path, ec);
	return !ec;
}

struct UnusedFile {
	fs::path path;
	std::time_t mtime;
	uint64_t bytes;
};

uint64_t saturatingAdd(uint64_t a, uint64_t b) {
	if (a > UINT64_MAX - b)
		return UINT64_MAX;
	return a + b;
}

bool spaceIsTight(uint64_t avail, uint64_t total, uint64_t min_free_bytes, double min_free_ratio) {
	if (min_free_bytes > 0 && avail < min_free_bytes)
		return true;
	if (min_free_ratio > 0.0 && total > 0 &&
		static_cast<double>(avail) / static_cast<double>(total) < min_free_ratio)
		return true;
	return false;
}

} // namespace

bool httpImageCacheSpace(const std::string &cache_dir, uint64_t *avail_bytes, uint64_t *total_bytes) {
	if (avail_bytes)
		*avail_bytes = 0;
	if (total_bytes)
		*total_bytes = 0;
	boost::system::error_code ec;
	if (!fs::is_directory(cache_dir, ec) || ec)
		return false;
	struct statvfs st;
	if (statvfs(cache_dir.c_str(), &st) != 0)
		return false;
	uint64_t frag = st.f_frsize ? static_cast<uint64_t>(st.f_frsize)
								: static_cast<uint64_t>(st.f_bsize);
	if (frag == 0)
		return false;
	auto blocks_to_bytes = [frag](uint64_t blocks) -> uint64_t {
		if (blocks > UINT64_MAX / frag)
			return UINT64_MAX;
		return blocks * frag;
	};
	const uint64_t avail = blocks_to_bytes(static_cast<uint64_t>(st.f_bavail));
	const uint64_t total = blocks_to_bytes(static_cast<uint64_t>(st.f_blocks));
	if (avail_bytes)
		*avail_bytes = avail;
	if (total_bytes)
		*total_bytes = total;
	return true;
}

bool httpImageCacheUnderPressure(const std::string &cache_dir, uint64_t min_free_bytes, double min_free_ratio) {
	uint64_t avail = 0;
	uint64_t total = 0;
	if (!httpImageCacheSpace(cache_dir, &avail, &total))
		return false;
	return spaceIsTight(avail, total, min_free_bytes, min_free_ratio);
}

bool discardHttpImageCacheFile(const std::string &cache_dir, const std::string &file_path) {
	if (cache_dir.empty() || file_path.empty())
		return false;
	const fs::path file(file_path);
	if (!isManagedCacheFile(cache_dir, file))
		return false;
	return removeRegularFile(file);
}

HttpImageCacheSweepResult sweepHttpImageCache(const HttpImageCachePolicy &policy) {
	HttpImageCacheSweepResult result;
	boost::system::error_code ec;
	if (policy.cache_dir.empty() || !fs::is_directory(policy.cache_dir, ec) || ec)
		return result;

	uint64_t avail = 0;
	uint64_t total = 0;
	const bool have_space = httpImageCacheSpace(policy.cache_dir, &avail, &total);
	if (have_space)
		result.pressure = spaceIsTight(avail, total, policy.min_free_bytes, policy.min_free_ratio);

	std::vector<UnusedFile> young;
	const fs::directory_iterator end;
	for (fs::directory_iterator it(policy.cache_dir, ec); !ec && it != end; it.increment(ec)) {
		const fs::path path = it->path();
		const fs::file_status st = fs::symlink_status(path, ec);
		if (ec || !fs::is_regular_file(st)) {
			ec.clear();
			continue;
		}
		if (!isManagedCacheFile(policy.cache_dir, path))
			continue;
		const std::string base = path.filename().string();
		if (policy.live_names.find(base) != policy.live_names.end()) {
			++result.kept_live;
			continue;
		}
		std::time_t mtime = 0;
		mtime = fs::last_write_time(path, ec);
		if (ec) {
			ec.clear();
			continue;
		}
		uintmax_t sz = 0;
		sz = fs::file_size(path, ec);
		if (ec) {
			ec.clear();
			sz = 0;
		}
		const bool aged = mtime <= policy.now_sec &&
			(policy.now_sec - mtime) >= policy.max_age_sec;
		if (aged) {
			if (removeRegularFile(path)) {
				++result.removed_aged;
				if (have_space)
					avail = saturatingAdd(avail, static_cast<uint64_t>(sz));
			}
			continue;
		}
		UnusedFile unused;
		unused.path = path;
		unused.mtime = mtime;
		unused.bytes = static_cast<uint64_t>(sz);
		young.push_back(unused);
	}

	if (have_space)
		result.pressure = spaceIsTight(avail, total, policy.min_free_bytes, policy.min_free_ratio);

	std::sort(young.begin(), young.end(), [](const UnusedFile &a, const UnusedFile &b) {
		if (a.mtime != b.mtime)
			return a.mtime < b.mtime;
		return a.path.filename().string() < b.path.filename().string();
	});

	for (const UnusedFile &file : young) {
		if (!have_space || !result.pressure) {
			++result.kept_young;
			continue;
		}
		if (!removeRegularFile(file.path)) {
			++result.kept_young;
			continue;
		}
		++result.removed_pressure;
		avail = saturatingAdd(avail, file.bytes);
		result.pressure = spaceIsTight(avail, total, policy.min_free_bytes, policy.min_free_ratio);
	}
	return result;
}
