/*
	DragHandle.cpp -- Handle widget with mouse control

	This file is based on controls in the NanoGUI souce.

	Please refer to the NanoGUI Licence file NonoGUI-LICENSE.txt

	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#include <iostream>
#include <nanogui/theme.h>
#include <nanogui/opengl.h>
#include <nanogui/serializer/core.h>
#include <nanogui/entypo.h>
#include "propertymonitor.h"
#include "draghandle.h"

NAMESPACE_BEGIN(nanogui)

DragHandle::DragHandle(Widget *parent, PropertyMonitor *pm)
	: ToolButton(parent, ENTYPO_ICON_PLUS), mValue(0.0f), mHighlightedRange(std::make_pair(0.f, 0.f)), property_monitor(pm) {
	mHighlightColor = Color(255, 50, 255, 200);
		setCursor(nanogui::Cursor::Hand);
		setTextColor(Color(255,50,255, 255));
		setVisible(false);
		setFontSize(18);
}

Vector2i DragHandle::preferredSize(NVGcontext *) const {
	return Vector2i(48, 48);
}

bool DragHandle::mouseDragEvent(const Vector2i &p, const Vector2i & /* rel */,
							int /* button */, int /* modifiers */) {
	if (!mEnabled) {
		return false;
	}
	int x = p.x();
	if (x < 0) x = 0;
	if (x>parent()->size().x()) x = parent()->size().x();
	int y = p.y();
	if (y<parent()->theme()->mWindowHeaderHeight+1) y = parent()->theme()->mWindowHeaderHeight+1;
	if (y>parent()->size().y()) y = parent()->size().y();
	setPosition( Vector2i(x-size().x()/2,y-size().y()/2 ) );
	if (property_monitor) property_monitor->update(this);
	if (mCallback)
		mCallback(mValue);
	return true;
}

bool DragHandle::mouseButtonEvent(const Vector2i &p, int /* button */, bool down, int /* modifiers */) {
	if (!mEnabled) {
		std::cout << "not enabled\n";
		return false;
	}
	if (down) {
		int x = p.x();
		if (x < 0) x = 0;
		if (x>parent()->size().x()) x = parent()->size().x();
		int y = p.y();
		if (y<parent()->theme()->mWindowHeaderHeight+1) y = parent()->theme()->mWindowHeaderHeight+1;
		if (y>parent()->size().y()) y = parent()->size().y();
		setPosition( Vector2i(x-size().x()/2,y-size().y()/2 ) );
	}
	if (mCallback)
		mCallback(mValue);
	if (mFinalCallback && !down)
		mFinalCallback(mValue);
	return true;
}

void DragHandle::draw(NVGcontext* ctx) {
	int fontSize = mFontSize == -1 ? mTheme->mButtonFontSize : mFontSize;
	nvgFontSize(ctx, 24);
	nvgFontFace(ctx, "sans-bold");
	float tw = nvgTextBounds(ctx, 0,0, mCaption.c_str(), nullptr, nullptr);

	Vector2f center = mPos.cast<float>() + mSize.cast<float>() * 0.5f;
	Vector2f textPos(center.x() - tw * 0.5f, center.y() - 1);
	NVGcolor textColor =
	mTextColor.w() == 0 ? mTheme->mTextColor : mTextColor;
	if (mIcon) {
		auto icon = utf8(mIcon);

		float iw, ih = fontSize*2;
		if (nvgIsFontIcon(mIcon)) {
			ih *= 1.5f;
			nvgFontSize(ctx, ih);
			nvgFontFace(ctx, "icons");
			iw = nvgTextBounds(ctx, 0, 0, icon.data(), nullptr, nullptr);
		} else {
			int w, h;
			ih *= 0.9f;
			nvgImageSize(ctx, mIcon, &w, &h);
			iw = w * ih / h;
		}
		if (mCaption != "")
			iw += mSize.y() * 0.15f;
		nvgFillColor(ctx, textColor);
		nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		Vector2f iconPos = center;
		iconPos.y() -= 1;

		if (mIconPosition == IconPosition::LeftCentered) {
			iconPos.x() -= (tw + iw) * 0.5f;
			textPos.x() += iw * 0.5f;
		} else if (mIconPosition == IconPosition::RightCentered) {
			textPos.x() -= iw * 0.5f;
			iconPos.x() += tw * 0.5f;
		} else if (mIconPosition == IconPosition::Left) {
			iconPos.x() = mPos.x() + 8;
		} else if (mIconPosition == IconPosition::Right) {
			iconPos.x() = mPos.x() + mSize.x() - iw - 8;
		}

		if (nvgIsFontIcon(mIcon)) {
			nvgText(ctx, iconPos.x(), iconPos.y()+1, icon.data(), nullptr);
		} else {
			NVGpaint imgPaint = nvgImagePattern(ctx,
					iconPos.x(), iconPos.y() - ih/2, iw, ih, 0, mIcon, mEnabled ? 0.5f : 0.25f);

			nvgFillPaint(ctx, imgPaint);
			nvgFill(ctx);
		}
	}
	return;
	//Vector2f center = mPos.cast<float>() + mSize.cast<float>() * 0.5f;
	Vector2f knobPos(center.x(), center.y() + 0.5f);
	float kr = (int)(mSize.y()*0.5f);
	NVGpaint bg = nvgBoxGradient(ctx,
		mPos.x(), center.y() - 3 + 1, mSize.x(), 6, 3, 3, Color(0, mEnabled ? 32 : 10), Color(0, mEnabled ? 128 : 210));
/*
	nvgBeginPath(ctx);
	nvgRoundedRect(ctx, mPos.x(), center.y() - 3 + 1, mSize.x(), 6, 2);
	nvgFillPaint(ctx, bg);
	nvgFill(ctx);

	if (mHighlightedRange.second != mHighlightedRange.first) {
		nvgBeginPath(ctx);
		nvgRoundedRect(ctx, mPos.x() + mHighlightedRange.first * mSize.x(), center.y() - 3 + 1, mSize.x() * (mHighlightedRange.second-mHighlightedRange.first), 6, 2);
		nvgFillColor(ctx, mHighlightColor);
		nvgFill(ctx);
	}
*/

	NVGpaint knobShadow = nvgRadialGradient(ctx,
		knobPos.x(), knobPos.y(), kr-3, kr+3, Color(0, 64), mTheme->mTransparent);
	nvgBeginPath(ctx);
	nvgRect(ctx, knobPos.x() - kr - 5, knobPos.y() - kr - 5, kr*2+10, kr*2+10+3);
	nvgCircle(ctx, knobPos.x(), knobPos.y(), kr);
	nvgPathWinding(ctx, NVG_HOLE);
	nvgFillPaint(ctx, knobShadow);
	nvgFill(ctx);

	NVGpaint knob = nvgLinearGradient(ctx,
		mPos.x(), center.y() - kr, mPos.x(), center.y() + kr,
		mTheme->mBorderLight, mTheme->mBorderMedium);
	NVGpaint knobReverse = nvgLinearGradient(ctx,
		mPos.x(), center.y() - kr, mPos.x(), center.y() + kr,
		mTheme->mBorderMedium,
		mTheme->mBorderLight);

	nvgBeginPath(ctx);
	nvgCircle(ctx, knobPos.x(), knobPos.y(), kr);
	nvgStrokeColor(ctx, mTheme->mBorderDark);
	nvgFillPaint(ctx, knob);
	nvgStroke(ctx);
	nvgFill(ctx);
	nvgBeginPath(ctx);
	nvgCircle(ctx, knobPos.x(), knobPos.y(), kr/2);
	nvgFillColor(ctx, Color(150, mEnabled ? 255 : 100));
	nvgStrokePaint(ctx, knobReverse);
	nvgStroke(ctx);
	nvgFill(ctx);
}

/*
void DragHandle::save(Serializer &s) const {
	Widget::save(s);
	s.set("value", mValue);
	s.set("highlightedRange", mHighlightedRange);
	s.set("highlightColor", mHighlightColor);
}

bool DragHandle::load(Serializer &s) {
	if (!Widget::load(s)) return false;
	if (!s.get("value", mValue)) return false;
	if (!s.get("highlightedRange", mHighlightedRange)) return false;
	if (!s.get("highlightColor", mHighlightColor)) return false;
	return true;
}
*/

NAMESPACE_END(nanogui)
