/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef __SCREENCAPTURE_H__
#define __SCREENCAPTURE_H__

#include <string>

bool writeFramebufferToPng(const std::string &filename, int width, int height);

#endif
