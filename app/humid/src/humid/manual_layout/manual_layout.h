/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
/*
		manual_layout.h -- a basic layout manager for nanogui

		NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
		The widget drawing code is based on the NanoVG demo application
		by Mikko Mononen.

		Refer to NanoGUI-LICENSE.txt for details on the nanogui license.

	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#pragma once

#include <nanogui/compat.h>
#include <nanogui/object.h>
#include <unordered_map>
#include <nanogui/layout.h>

NAMESPACE_BEGIN(nanogui)
/**
 * \brief Simple manual layout for hand drawn panels
 */
class ManualLayout : public Layout {
public:
		ManualLayout(Vector2i requested_size);

		/* Implementation of the layout interface */
		virtual Vector2i preferredSize(NVGcontext *ctx, const Widget *widget) const override;
		virtual void performLayout(NVGcontext *ctx, Widget *widget) const override;
protected:
	Vector2i user_size;
};

NAMESPACE_END(nanogui)
