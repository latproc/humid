//
//  image_cache.h
//  Project: humid
//
//	HTTP images are saved under the process cwd as cache/<name>.<ext>
//	(on the setup machines that directory is a symlink onto /dev/shm).
//	Textures already expire from the GL cache; these helpers expire the
//	matching files so the shm filesystem cannot fill for the whole uptime.
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __image_cache_h__
#define __image_cache_h__

#include <cstdint>
#include <ctime>
#include <set>
#include <string>

// cwd-relative directory getImageId() writes into.
extern const char kHttpImageCacheDir[];

// Orphan files (no live texture) older than this are removed.
const std::time_t kHttpImageCacheMaxAgeSec = 600;

// Sweep on this interval, and sooner while the filesystem is under the floor.
const uint64_t kHttpImageCacheSweepIntervalUs = 180ull * 1000ull * 1000ull;

// Free-space floor. Pressure is cleared only when both are satisfied.
const uint64_t kHttpImageCacheMinFreeBytes = 64ull * 1024ull * 1024ull;
const double kHttpImageCacheMinFreeRatio = 0.20;

struct HttpImageCachePolicy {
	std::string cache_dir;
	// Basenames currently backing a loaded texture. Those files stay.
	std::set<std::string> live_names;
	std::time_t now_sec = 0;
	std::time_t max_age_sec = kHttpImageCacheMaxAgeSec;
	uint64_t min_free_bytes = kHttpImageCacheMinFreeBytes;
	double min_free_ratio = kHttpImageCacheMinFreeRatio;
};

struct HttpImageCacheSweepResult {
	int removed_aged = 0;
	int removed_pressure = 0;
	int kept_live = 0;
	int kept_young = 0;
	bool pressure = false;
};

// Bytes available and total for the filesystem that holds cache_dir.
// Returns false when the directory is missing or statvfs fails.
bool httpImageCacheSpace(const std::string &cache_dir, uint64_t *avail_bytes, uint64_t *total_bytes);

bool httpImageCacheUnderPressure(const std::string &cache_dir, uint64_t min_free_bytes, double min_free_ratio);

// Remove one file Humid wrote. Refuses anything that is not a regular image
// file sitting directly in cache_dir (no symlink, no subdirectory, no other
// filesystem object that happens to share the directory).
bool discardHttpImageCacheFile(const std::string &cache_dir, const std::string &file_path);

// Age sweep, then oldest-first removal of non-live image files while the
// filesystem is under the free-space floor. Does not create cache_dir.
HttpImageCacheSweepResult sweepHttpImageCache(const HttpImageCachePolicy &policy);

#endif
