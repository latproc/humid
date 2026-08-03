//
//  platformdetect.cpp
//  Project: humid
//
//  All rights reserved. Use of this source code is governed by the
//  3-clause BSD License in LICENSE.txt.

#include "platformdetect.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <sys/utsname.h>
#  include <unistd.h>
#endif

namespace {

std::string read_first_line(const char *path) {
	std::ifstream in(path);
	if (!in)
		return std::string();
	std::string line;
	std::getline(in, line);
	// device-tree model is often not newline-terminated and may contain NULs
	while (!line.empty() && (line.back() == '\0' || line.back() == '\n' || line.back() == '\r'))
		line.pop_back();
	return line;
}

bool file_exists(const char *path) {
	std::ifstream in(path);
	return static_cast<bool>(in);
}

} // namespace

HumidPlatformInfo detectHumidPlatform() {
	HumidPlatformInfo info;
	info.is_raspberry_pi = false;
	info.is_arm = false;
	info.wayland_session = false;
	info.x11_session = false;

#if defined(NANOGUI_GLES)
	info.built_with_gles = true;
#elif defined(HUMID_USE_GLES) && HUMID_USE_GLES
	info.built_with_gles = true;
#else
	info.built_with_gles = false;
#endif

#if defined(_WIN32)
	info.os_name = "Windows";
	SYSTEM_INFO si;
	GetNativeSystemInfo(&si);
	switch (si.wProcessorArchitecture) {
	case PROCESSOR_ARCHITECTURE_AMD64: info.arch = "x86_64"; break;
	case PROCESSOR_ARCHITECTURE_ARM64: info.arch = "aarch64"; info.is_arm = true; break;
	case PROCESSOR_ARCHITECTURE_ARM:   info.arch = "arm"; info.is_arm = true; break;
	default: info.arch = "unknown"; break;
	}
#else
	struct utsname uts;
	if (uname(&uts) == 0) {
		info.os_name = uts.sysname;
		info.arch = uts.machine;
	} else {
		info.os_name = "unknown";
		info.arch = "unknown";
	}
	if (info.arch.find("aarch64") != std::string::npos ||
	    info.arch.find("arm64") != std::string::npos ||
	    info.arch.find("armv") == 0 ||
	    info.arch == "arm") {
		info.is_arm = true;
	}

	// Raspberry Pi device-tree model (Linux)
	info.device_model = read_first_line("/proc/device-tree/model");
	if (info.device_model.find("Raspberry") != std::string::npos ||
	    info.device_model.find("raspberry") != std::string::npos) {
		info.is_raspberry_pi = true;
	} else if (file_exists("/etc/rpi-issue") || file_exists("/usr/bin/raspi-config")) {
		info.is_raspberry_pi = true;
		if (info.device_model.empty())
			info.device_model = "Raspberry Pi (detected via OS markers)";
	}

	const char *wayland = std::getenv("WAYLAND_DISPLAY");
	const char *display = std::getenv("DISPLAY");
	info.wayland_session = (wayland && wayland[0]);
	info.x11_session = (display && display[0]);
#endif

	if (info.wayland_session)
		info.presentation = "wayland";
	else if (info.x11_session)
		info.presentation = "x11";
	else
		info.presentation = "unknown";

	return info;
}

void logHumidPlatform(const HumidPlatformInfo &info) {
	std::ostringstream oss;
	oss << "Humid platform: os=" << info.os_name
	    << " arch=" << info.arch
	    << " gl=" << (info.built_with_gles ? "OpenGL-ES-3.0" : "OpenGL")
	    << " presentation=" << info.presentation;
	if (info.is_raspberry_pi)
		oss << " rpi=yes";
	if (!info.device_model.empty())
		oss << " model=\"" << info.device_model << "\"";
	std::cout << oss.str() << "\n" << std::flush;

	// Helpful guidance when the binary looks mismatched for the host
	if (info.is_raspberry_pi && !info.built_with_gles) {
		std::cout << "WARNING: running on Raspberry Pi but this binary was built "
		             "with desktop OpenGL. Rebuild with NANOGUI_USE_GLES=ON "
		             "(auto-selected on aarch64/RPi at configure time).\n"
		          << std::flush;
	}
	if (!info.is_arm && info.built_with_gles) {
		std::cout << "NOTE: this binary was built with OpenGL ES; desktop panels "
		             "normally use desktop OpenGL.\n"
		          << std::flush;
	}
}
