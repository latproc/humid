/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
#include <nanogui/layout.h>
#include <nanogui/widget.h>
#include <nanogui/window.h>
#include <nanogui/theme.h>
#include <nanogui/label.h>
#include <numeric>
#include "manuallayout.h"

NAMESPACE_BEGIN(nanogui)

ManualLayout::ManualLayout(Vector2i requested_size) : user_size(requested_size) {
}

Vector2i ManualLayout::preferredSize(NVGcontext *ctx, const Widget *widget) const {
		return user_size;
}

void ManualLayout::performLayout(NVGcontext *ctx, Widget *widget) const {
	for (auto w : widget->children()) {
		if (!w->visible())
				continue;
		w->performLayout(ctx);
	}
}
NAMESPACE_END(nanogui)
