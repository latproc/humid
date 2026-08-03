//
//  platformdetect.h
//  Project: humid
//
//  All rights reserved. Use of this source code is governed by the
//  3-clause BSD License in LICENSE.txt.

#pragma once

#include <string>

/** Build- and runtime platform facts used to select panel behaviour. */
struct HumidPlatformInfo {
	std::string arch;           // e.g. aarch64, x86_64
	std::string os_name;        // e.g. Linux, Darwin
	std::string device_model;   // e.g. Raspberry Pi 4 Model B Rev 1.4
	bool is_raspberry_pi;
	bool is_arm;
	bool built_with_gles;       // compile-time: OpenGL ES vs desktop GL
	bool wayland_session;       // WAYLAND_DISPLAY set
	bool x11_session;           // DISPLAY set
	std::string presentation;   // "wayland", "x11", "unknown", ...
};

/** Collect platform info (cheap; safe to call early in main). */
HumidPlatformInfo detectHumidPlatform();

/** One-line and multi-line log of platform / GL build choices. */
void logHumidPlatform(const HumidPlatformInfo &info);
