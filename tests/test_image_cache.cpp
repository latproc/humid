//
//  test_image_cache.cpp
//  Project: humid
//
//	Filesystem checks for the HTTP image cache sweeper. No OpenGL.

#include "image_cache.h"

#include <boost/filesystem.hpp>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

namespace fs = boost::filesystem;

static int g_failures = 0;

static void check(bool cond, const char *expr, int line) {
	if (!cond) {
		std::cerr << "FAIL " << line << ": " << expr << "\n";
		++g_failures;
	}
}

#define CHECK(cond) check(static_cast<bool>(cond), #cond, __LINE__)

static void writeFile(const fs::path &path, const std::string &body) {
	const fs::path parent = path.parent_path();
	if (!parent.empty())
		fs::create_directories(parent);
	std::ofstream out(path.string().c_str(), std::ios::binary);
	out << body;
	CHECK(out.good());
}

static HttpImageCachePolicy policyAt(const std::string &dir, std::time_t now) {
	HttpImageCachePolicy policy;
	policy.cache_dir = dir;
	policy.now_sec = now;
	policy.max_age_sec = kHttpImageCacheMaxAgeSec;
	policy.min_free_bytes = 0;
	policy.min_free_ratio = 0;
	return policy;
}

static void testMissingDirectory() {
	const HttpImageCacheSweepResult result = sweepHttpImageCache(policyAt("missing-cache", 1700000000));
	CHECK(result.removed_aged == 0);
	CHECK(result.removed_pressure == 0);
	CHECK(!httpImageCacheUnderPressure("missing-cache", 1, 0.0));
	CHECK(!discardHttpImageCacheFile("missing-cache", "missing-cache/page001.png"));
}

static void testAgeKeepsLiveYoungAndForeign() {
	const std::time_t now = 1700000000;
	const std::string dir = "age/cache";
	writeFile(dir + "/old.png", "old");
	writeFile(dir + "/young.png", "young");
	writeFile(dir + "/live.png", "live");
	writeFile(dir + "/notes.txt", "notes");
	writeFile(dir + "/.hidden.png", "hidden");
	writeFile("age/cache/nested/page002.png", "nested");
	fs::last_write_time(dir + "/old.png", now - kHttpImageCacheMaxAgeSec - 1);
	fs::last_write_time(dir + "/young.png", now - 30);
	fs::last_write_time(dir + "/live.png", now - 5000);
	fs::last_write_time(dir + "/notes.txt", now - 5000);
	fs::last_write_time(dir + "/.hidden.png", now - 5000);
	fs::last_write_time("age/cache/nested/page002.png", now - 5000);

	HttpImageCachePolicy policy = policyAt(dir, now);
	policy.live_names.insert("live.png");
	const HttpImageCacheSweepResult result = sweepHttpImageCache(policy);
	CHECK(result.removed_aged == 1);
	CHECK(result.kept_live == 1);
	CHECK(result.kept_young == 1);
	CHECK(!fs::exists(dir + "/old.png"));
	CHECK(fs::exists(dir + "/young.png"));
	CHECK(fs::exists(dir + "/live.png"));
	CHECK(fs::exists(dir + "/notes.txt"));
	CHECK(fs::exists(dir + "/.hidden.png"));
	CHECK(fs::exists("age/cache/nested/page002.png"));
}

static void testSymlinkAndDiscardGuards() {
	writeFile("secret.png", "secret");
	fs::create_directories("sym/cache");
	fs::create_symlink("../../secret.png", "sym/cache/page001.png");
	writeFile("sym/cache/old.png", "old");
	const std::time_t now = 1700000000;
	fs::last_write_time("sym/cache/old.png", now - kHttpImageCacheMaxAgeSec - 5);

	const HttpImageCacheSweepResult result = sweepHttpImageCache(policyAt("sym/cache", now));
	CHECK(result.removed_aged == 1);
	CHECK(!fs::exists("sym/cache/old.png"));
	CHECK(fs::is_symlink("sym/cache/page001.png"));
	CHECK(fs::exists("secret.png"));

	writeFile("sym/cache/page003.png", "page");
	CHECK(discardHttpImageCacheFile("sym/cache", "sym/cache/page003.png"));
	CHECK(!fs::exists("sym/cache/page003.png"));
	CHECK(!discardHttpImageCacheFile("sym/cache", "sym/cache/page001.png"));
	CHECK(!discardHttpImageCacheFile("sym/cache", "secret.png"));
	CHECK(!discardHttpImageCacheFile("sym/cache", "sym/cache/../../secret.png"));
	CHECK(fs::exists("secret.png"));
	CHECK(!discardHttpImageCacheFile("sym/cache", "sym/cache/nested/page002.png"));
}

static void testCacheDirectorySymlink() {
	fs::create_directories("real_store");
	fs::create_directory_symlink("real_store", "cache");
	writeFile("cache/old.png", "old");
	writeFile("cache/young.jpg", "young");
	const std::time_t now = 1700000000;
	fs::last_write_time("cache/old.png", now - kHttpImageCacheMaxAgeSec - 1);
	fs::last_write_time("cache/young.jpg", now - 5);
	HttpImageCachePolicy policy = policyAt("cache", now);
	policy.live_names.insert("young.jpg");
	const HttpImageCacheSweepResult result = sweepHttpImageCache(policy);
	CHECK(result.removed_aged == 1);
	CHECK(result.kept_live == 1);
	CHECK(fs::is_symlink("cache"));
	CHECK(fs::is_directory("real_store"));
	CHECK(!fs::exists("real_store/old.png"));
	CHECK(fs::exists("real_store/young.jpg"));
	fs::remove("cache");
	CHECK(fs::is_directory("real_store"));
}

static void testPressureDropsOldestUnused() {
	const std::string dir = "pressure/cache";
	const std::string payload(1000, 'x');
	writeFile(dir + "/live.png", payload);
	writeFile(dir + "/a.png", payload);
	writeFile(dir + "/b.png", payload);
	writeFile(dir + "/c.png", payload);
	const std::time_t now = 1700000000;
	fs::last_write_time(dir + "/live.png", now - 50);
	fs::last_write_time(dir + "/a.png", now - 40);
	fs::last_write_time(dir + "/b.png", now - 30);
	fs::last_write_time(dir + "/c.png", now - 20);

	uint64_t avail = 0;
	uint64_t total = 0;
	CHECK(httpImageCacheSpace(dir, &avail, &total));
	CHECK(total > 0);

	HttpImageCachePolicy policy = policyAt(dir, now);
	policy.live_names.insert("live.png");
	policy.min_free_bytes = avail + 1500;
	policy.min_free_ratio = 0;
	const HttpImageCacheSweepResult result = sweepHttpImageCache(policy);
	CHECK(result.removed_pressure == 2);
	CHECK(result.removed_aged == 0);
	CHECK(result.kept_live == 1);
	CHECK(result.kept_young == 1);
	CHECK(fs::exists(dir + "/live.png"));
	CHECK(!fs::exists(dir + "/a.png"));
	CHECK(!fs::exists(dir + "/b.png"));
	CHECK(fs::exists(dir + "/c.png"));
}

static void testPressureRatioKeepsLive() {
	const std::string dir = "ratio/cache";
	writeFile(dir + "/live.png", "live");
	writeFile(dir + "/a.png", "a");
	writeFile(dir + "/b.jpeg", "b");
	const std::time_t now = 1700000000;
	fs::last_write_time(dir + "/live.png", now - 10);
	fs::last_write_time(dir + "/a.png", now - 10);
	fs::last_write_time(dir + "/b.jpeg", now - 10);
	HttpImageCachePolicy policy = policyAt(dir, now);
	policy.live_names.insert("live.png");
	policy.min_free_bytes = 0;
	policy.min_free_ratio = 2.0;
	const HttpImageCacheSweepResult result = sweepHttpImageCache(policy);
	CHECK(result.removed_pressure == 2);
	CHECK(result.kept_live == 1);
	CHECK(fs::exists(dir + "/live.png"));
	CHECK(!fs::exists(dir + "/a.png"));
	CHECK(!fs::exists(dir + "/b.jpeg"));
	CHECK(httpImageCacheUnderPressure(dir, 0, 2.0));
	CHECK(!httpImageCacheUnderPressure(dir, 0, 0.0));
}

int main() {
	char tmpl[] = "/tmp/humid-image-cache-XXXXXX";
	if (!mkdtemp(tmpl)) {
		std::perror("mkdtemp");
		return 1;
	}
	if (chdir(tmpl) != 0) {
		std::perror("chdir");
		return 1;
	}

	testMissingDirectory();
	testAgeKeepsLiveYoungAndForeign();
	testSymlinkAndDiscardGuards();
	testCacheDirectorySymlink();
	testPressureDropsOldestUnused();
	testPressureRatioKeepsLive();

	if (g_failures) {
		std::cerr << g_failures << " failure(s) under " << tmpl << "\n";
		return 1;
	}
	if (chdir("/tmp") == 0)
		fs::remove_all(tmpl);
	std::cout << "image cache tests passed\n";
	return 0;
}
