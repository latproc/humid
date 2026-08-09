//
//  editordocview.cpp
//  Project: humid
//

#include "editordocview.h"
#include "editor.h"
#include "editorgui.h"
#include "resourcemanager.h"
#include "propertyformhelper.h"
#include "helper.h"
#include "structure.h"
#include "linkableobject.h"

#include <nanogui/opengl.h>
#include <nanogui/screen.h>

#include <cstdio>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <sstream>

namespace {

std::string joinPageUrl(const std::string &source, int page) {
	if (source.empty() || page < 1)
		return std::string();
	std::string base = source;
	// If source already looks like a full image path, use as-is for page 1 only.
	const std::string lower = base;
	auto ends_with = [](const std::string &s, const char *suf) {
		const size_t n = std::strlen(suf);
		return s.size() >= n && s.compare(s.size() - n, n, suf) == 0;
	};
	if (ends_with(lower, ".png") || ends_with(lower, ".jpg") || ends_with(lower, ".jpeg") ||
		ends_with(lower, ".PNG") || ends_with(lower, ".JPG")) {
		return page == 1 ? base : std::string();
	}
	if (!base.empty() && base.back() != '/')
		base.push_back('/');
	char buf[32];
	std::snprintf(buf, sizeof(buf), "page-%03d.png", page);
	return base + buf;
}

} // namespace

const std::map<std::string, std::string> &EditorDocView::property_map() const {
	auto structure_class = findClass("DOCVIEW");
	assert(structure_class);
	return structure_class->property_map();
}

const std::map<std::string, std::string> &EditorDocView::reverse_property_map() const {
	auto structure_class = findClass("DOCVIEW");
	assert(structure_class);
	return structure_class->reverse_property_map();
}

EditorDocView::EditorDocView(NamedObject *owner, Widget *parent, const std::string nam,
							 LinkableProperty *lp, GLuint image_id)
	: ImageView(parent, image_id), EditorWidget(owner, "DOCVIEW", nam, this, lp) {
	if (mImageID)
		ResourceManager::manage(mImageID);
	setInteractive(true);
	// Wiring drawings: no pixel grid / pixel-info overlays.
	setGridThreshold(-1.f);
	setPixelInfoThreshold(-1.f);
}

EditorDocView::~EditorDocView() {
	if (mImageID)
		ResourceManager::release(mImageID);
}

void EditorDocView::setInteractive(bool value) {
	m_interactive = value;
	setFixedOffset(!value);
	setFixedScale(!value);
}

std::string EditorDocView::pageUrl() const {
	return joinPageUrl(m_source, m_page);
}

void EditorDocView::bindImageId(GLuint img) {
	if (img == mImageID)
		return;
	if (mImageID && ResourceManager::release(mImageID) == 0) {
		if (EDITOR && EDITOR->gui())
			EDITOR->gui()->freeImage(mImageID);
	}
	if (img) {
		mImageID = ResourceManager::manage(img);
		updateImageParameters();
	} else {
		mImageID = 0;
		mImageSize = nanogui::Vector2i(0, 0);
	}
}

void EditorDocView::loadCurrentPage(bool reload) {
	const std::string url = pageUrl();
	if (url.empty()) {
		m_status = m_source.empty() ? "No source" : "Invalid page";
		bindImageId(0);
		return;
	}
	if (!EDITOR || !EDITOR->gui()) {
		m_status = "No GUI";
		return;
	}
	GLuint img = EDITOR->gui()->getImageId(url.c_str(), reload);
	if (!img) {
		m_status = "Load failed";
		std::cerr << "DOCVIEW: failed to load " << url << "\n";
		bindImageId(0);
		return;
	}
	m_status.clear();
	bindImageId(img);
	if (m_need_fit || reload) {
		fit();
		m_need_fit = false;
	}
}

void EditorDocView::setSource(const std::string &src, bool reload) {
	if (m_source == src && !reload)
		return;
	m_source = src;
	m_need_fit = true;
	if (m_page < 1)
		m_page = 1;
	if (m_page_count < 1)
		m_page_count = 1;
	if (m_page > m_page_count)
		m_page = m_page_count;
	loadCurrentPage(true);
}

void EditorDocView::setPage(int page, bool reload) {
	if (page < 1)
		page = 1;
	if (m_page_count > 0 && page > m_page_count)
		page = m_page_count;
	if (m_page == page && !reload)
		return;
	m_page = page;
	m_need_fit = true;
	loadCurrentPage(reload);
}

void EditorDocView::setPageCount(int n) {
	if (n < 1)
		n = 1;
	m_page_count = n;
	if (m_page > m_page_count)
		setPage(m_page_count, true);
}

void EditorDocView::nextPage() {
	if (m_page < m_page_count)
		setPage(m_page + 1, true);
}

void EditorDocView::prevPage() {
	if (m_page > 1)
		setPage(m_page - 1, true);
}

void EditorDocView::fitView() {
	fit();
	m_need_fit = false;
}

nanogui::Vector4i EditorDocView::prevButtonRect() const {
	const int bw = 56;
	const int bh = 28;
	const int pad = 8;
	return nanogui::Vector4i(pad, mSize.y() - bh - pad, bw, bh);
}

nanogui::Vector4i EditorDocView::nextButtonRect() const {
	const int bw = 56;
	const int bh = 28;
	const int pad = 8;
	return nanogui::Vector4i(pad + bw + 6, mSize.y() - bh - pad, bw, bh);
}

nanogui::Vector4i EditorDocView::fitButtonRect() const {
	const int bw = 48;
	const int bh = 28;
	const int pad = 8;
	return nanogui::Vector4i(mSize.x() - bw - pad, mSize.y() - bh - pad, bw, bh);
}

bool EditorDocView::hitRect(const nanogui::Vector4i &r, const nanogui::Vector2i &local_p) const {
	return local_p.x() >= r.x() && local_p.y() >= r.y() && local_p.x() < r.x() + r.z() &&
		   local_p.y() < r.y() + r.w();
}

bool EditorDocView::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down,
									 int modifiers) {
	using namespace nanogui;
	if (editorMouseButtonEvent(this, p, button, down, modifiers)) {
		// Run mode: chrome buttons only — no mouse pan/drag on the image.
		if (!EDITOR || !EDITOR->isEditMode()) {
			const Vector2i local = toLocal(p);
			if (down && button == GLFW_MOUSE_BUTTON_1) {
				// Take focus so arrow / +/- keys go here.
				requestFocus();
				if (hitRect(prevButtonRect(), local)) {
					prevPage();
					return true;
				}
				if (hitRect(nextButtonRect(), local)) {
					nextPage();
					return true;
				}
				if (hitRect(fitButtonRect(), local)) {
					fitView();
					return true;
				}
				return true; // consume click; do not start ImageView drag
			}
		}
		return Widget::mouseButtonEvent(p, button, down, modifiers);
	}
	return true;
}

bool EditorDocView::mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel,
									 int button, int modifiers) {
	if (editorMouseMotionEvent(this, p, rel, button, modifiers))
		return Widget::mouseMotionEvent(p, rel, button, modifiers);
	return true;
}

bool EditorDocView::mouseDragEvent(const nanogui::Vector2i & /*p*/, const nanogui::Vector2i & /*rel*/,
								   int /*button*/, int /*modifiers*/) {
	// Operators use keyboard pan; mouse drag pan is disabled.
	return false;
}

bool EditorDocView::mouseEnterEvent(const Vector2i &p, bool enter) {
	if (editorMouseEnterEvent(this, p, enter))
		return Widget::mouseEnterEvent(p, enter);
	return true;
}

bool EditorDocView::scrollEvent(const nanogui::Vector2i & /*p*/, const nanogui::Vector2f & /*rel*/) {
	// Zoom is keyboard (+/-) only — ignore wheel.
	return false;
}

bool EditorDocView::keyboardEvent(int key, int scancode, int action, int modifiers) {
	using namespace nanogui;
	if (EDITOR && EDITOR->isEditMode())
		return Widget::keyboardEvent(key, scancode, action, modifiers);
	if (action != GLFW_PRESS && action != GLFW_REPEAT)
		return Widget::keyboardEvent(key, scancode, action, modifiers);

	const bool shift = (modifiers & GLFW_MOD_SHIFT) != 0;
	const bool ctrl = (modifiers & GLFW_MOD_CONTROL) != 0;
	// Large steps for drawings; Shift = even larger (roughly half a widget).
	const float step = shift ? 220.f : (ctrl ? 140.f : 100.f);

	// Page navigation — not on arrows (arrows pan).
	if (key == GLFW_KEY_PAGE_UP || key == GLFW_KEY_LEFT_BRACKET) {
		prevPage();
		return true;
	}
	if (key == GLFW_KEY_PAGE_DOWN || key == GLFW_KEY_RIGHT_BRACKET) {
		nextPage();
		return true;
	}
	if (key == GLFW_KEY_HOME) {
		setPage(1, true);
		return true;
	}
	if (key == GLFW_KEY_END) {
		setPage(m_page_count, true);
		return true;
	}

	// Fit / centre
	if (key == GLFW_KEY_F || key == GLFW_KEY_0) {
		fitView();
		return true;
	}
	if (key == GLFW_KEY_C) {
		center();
		return true;
	}

	// Zoom about widget centre
	if (m_interactive && (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD)) {
		zoom(1, sizeF() * 0.5f);
		return true;
	}
	if (m_interactive && (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT)) {
		zoom(-1, sizeF() * 0.5f);
		return true;
	}

	// Pan (image content moves with arrow direction — natural for reading)
	if (m_interactive && !fixedOffset()) {
		if (key == GLFW_KEY_LEFT) {
			moveOffset(Vector2f(step, 0.f));
			return true;
		}
		if (key == GLFW_KEY_RIGHT) {
			moveOffset(Vector2f(-step, 0.f));
			return true;
		}
		if (key == GLFW_KEY_UP) {
			moveOffset(Vector2f(0.f, step));
			return true;
		}
		if (key == GLFW_KEY_DOWN) {
			moveOffset(Vector2f(0.f, -step));
			return true;
		}
	}

	return Widget::keyboardEvent(key, scancode, action, modifiers);
}

bool EditorDocView::keyboardCharacterEvent(unsigned int codepoint) {
	if (EDITOR && EDITOR->isEditMode())
		return Widget::keyboardCharacterEvent(codepoint);
	if (!m_interactive)
		return false;
	// Character path for + / - on some layouts
	if (codepoint == '+' || codepoint == '=') {
		zoom(1, sizeF() * 0.5f);
		return true;
	}
	if (codepoint == '-' || codepoint == '_') {
		zoom(-1, sizeF() * 0.5f);
		return true;
	}
	if (codepoint == 'f' || codepoint == 'F') {
		fitView();
		return true;
	}
	if (codepoint == 'c' || codepoint == 'C') {
		center();
		return true;
	}
	return false;
}

void EditorDocView::drawChrome(NVGcontext *ctx) {
	using namespace nanogui;
	auto drawBtn = [&](const Vector4i &r, const char *label, bool enabled) {
		float x = mPos.x() + r.x();
		float y = mPos.y() + r.y();
		float bw = (float)r.z();
		float bh = (float)r.w();
		nvgBeginPath(ctx);
		nvgRoundedRect(ctx, x, y, bw, bh, 4.f);
		nvgFillColor(ctx, enabled ? nvgRGBA(30, 80, 140, 220) : nvgRGBA(80, 80, 90, 160));
		nvgFill(ctx);
		nvgFontSize(ctx, 14.f);
		nvgFontFace(ctx, "sans-bold");
		nvgFillColor(ctx, nvgRGBA(255, 255, 255, enabled ? 255 : 180));
		nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgText(ctx, x + bw * 0.5f, y + bh * 0.5f, label, nullptr);
	};

	drawBtn(prevButtonRect(), "Prev", m_page > 1);
	drawBtn(nextButtonRect(), "Next", m_page < m_page_count);
	drawBtn(fitButtonRect(), "Fit", true);

	// Page label
	char label[64];
	if (!m_status.empty())
		std::snprintf(label, sizeof(label), "%s", m_status.c_str());
	else
		std::snprintf(label, sizeof(label), "%d / %d", m_page, m_page_count);

	const float lx = mPos.x() + 8.f + 56.f + 6.f + 56.f + 10.f;
	const float ly = mPos.y() + mSize.y() - 8.f - 14.f;
	nvgFontSize(ctx, 13.f);
	nvgFontFace(ctx, "sans");
	nvgFillColor(ctx, nvgRGBA(20, 20, 20, 220));
	nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);
	// soft backdrop
	float tw = nvgTextBounds(ctx, lx, ly, label, nullptr, nullptr);
	nvgBeginPath(ctx);
	nvgRoundedRect(ctx, lx - 4.f, ly - 16.f, tw + 8.f, 20.f, 3.f);
	nvgFillColor(ctx, nvgRGBA(255, 255, 255, 200));
	nvgFill(ctx);
	nvgFillColor(ctx, nvgRGBA(20, 30, 40, 255));
	nvgText(ctx, lx, ly, label, nullptr);
}

void EditorDocView::draw(NVGcontext *ctx) {
	using namespace nanogui;
	Widget::draw(ctx);
	nvgEndFrame(ctx);

	if (border)
		drawImageBorder(ctx);

	if (mImageID) {
		const Screen *screen = dynamic_cast<const Screen *>(this->window()->parent());
		if (screen) {
			Vector2f screenSize = screen->size().cast<float>();
			Vector2f scaleFactor = mScale * imageSizeF().cwiseQuotient(screenSize);
			Vector2f positionInScreen = absolutePosition().cast<float>();
			Vector2f positionAfterOffset = positionInScreen + mOffset;
			Vector2f imagePosition = positionAfterOffset.cwiseQuotient(screenSize);
			glEnable(GL_SCISSOR_TEST);
			float r = screen->pixelRatio();
			glScissor(positionInScreen.x() * r,
					  (screenSize.y() - positionInScreen.y() - size().y()) * r, size().x() * r,
					  size().y() * r);
			mShader.bind();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, mImageID);
			mShader.setUniform("image", 0);
			mShader.setUniform("scaleFactor", scaleFactor);
			mShader.setUniform("position", imagePosition);
			mShader.drawIndexed(GL_TRIANGLES, 0, 2);
			glDisable(GL_SCISSOR_TEST);
		}
	} else {
		// Empty state
		nvgBeginPath(ctx);
		nvgRect(ctx, mPos.x(), mPos.y(), (float)mSize.x(), (float)mSize.y());
		nvgFillColor(ctx, nvgRGBA(240, 242, 245, 255));
		nvgFill(ctx);
		nvgFontSize(ctx, 16.f);
		nvgFontFace(ctx, "sans");
		nvgFillColor(ctx, nvgRGBA(80, 90, 100, 255));
		nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		const char *msg = m_status.empty() ? "DOCVIEW: set source URL" : m_status.c_str();
		nvgText(ctx, mPos.x() + mSize.x() * 0.5f, mPos.y() + mSize.y() * 0.5f, msg, nullptr);
	}

	if (helpersVisible())
		drawHelpers(ctx);

	drawChrome(ctx);

	if (border)
		drawWidgetBorder(ctx);
	if (mSelected)
		drawSelectionBorder(ctx, mPos, mSize);
	else if (EDITOR && EDITOR->isEditMode())
		drawElementBorder(ctx, mPos, mSize);
}

void EditorDocView::getPropertyNames(std::list<std::string> &names) {
	EditorWidget::getPropertyNames(names);
	names.push_back("Source");
	names.push_back("Page");
	names.push_back("Pages");
	names.push_back("Scale");
	names.push_back("Interactive");
}

void EditorDocView::loadPropertyToStructureMap(std::map<std::string, std::string> &properties) {
	properties = property_map();
}

Value EditorDocView::getPropertyValue(const std::string &prop) {
	Value res = EditorWidget::getPropertyValue(prop);
	if (res != SymbolTable::Null)
		return res;
	if (prop == "Source")
		return Value(m_source, Value::t_string);
	if (prop == "Page")
		return m_page;
	if (prop == "Pages")
		return m_page_count;
	if (prop == "Scale")
		return scale();
	if (prop == "Interactive")
		return m_interactive;
	return SymbolTable::Null;
}

void EditorDocView::setProperty(const std::string &prop, const std::string value) {
	EditorWidget::setProperty(prop, value);
	if (prop == "Source") {
		setSource(value, true);
	} else if (prop == "Page") {
		setPage(std::atoi(value.c_str()), true);
	} else if (prop == "Pages") {
		setPageCount(std::atoi(value.c_str()));
	} else if (prop == "Scale") {
		setScale((float)std::atof(value.c_str()));
	} else if (prop == "Interactive") {
		bool interactive = false;
		Value v(value);
		if (v.asBoolean(interactive) || value == "1" || value == "0") {
			if (value == "1" || value == "0")
				interactive = value == "1";
			setInteractive(interactive);
		}
	} else if (prop == "Remote") {
		if (remote)
			remote->unlink(this);
		remote = EDITOR->gui()->findLinkableProperty(value);
		if (remote)
			remote->link(new LinkableText(this));
	}
}

void EditorDocView::loadProperties(PropertyFormHelper *properties) {
	EditorWidget::loadProperties(properties);
	nanogui::Widget *w = dynamic_cast<nanogui::Widget *>(this);
	if (!w)
		return;
	properties->addVariable<std::string>(
		"Source",
		[&](std::string value) mutable { setSource(value, true); },
		[&]() -> std::string { return source(); });
	properties->addVariable<int>(
		"Page", [&](int value) mutable { setPage(value, true); },
		[&]() -> int { return page(); });
	properties->addVariable<int>(
		"Pages", [&](int value) mutable { setPageCount(value); },
		[&]() -> int { return pageCount(); });
	properties->addVariable<float>(
		"Scale",
		[&](float value) mutable {
			setScale(value);
			center();
		},
		[&]() -> float { return scale(); });
	properties->addVariable<bool>(
		"Interactive", [&](bool value) mutable { setInteractive(value); },
		[&]() -> bool { return interactive(); });
	properties->addGroup("Remote");
	properties->addVariable<std::string>(
		"Remote object",
		[&, this](std::string value) {
			LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(value);
			this->setRemoteName(value);
			if (remote)
				remote->unlink(this);
			remote = lp;
			if (lp)
				lp->link(new LinkableText(this));
		},
		[&]() -> std::string {
			if (remote)
				return remote->tagName();
			if (getDefinition()) {
				const Value &rmt_v = getDefinition()->getValue("remote");
				if (rmt_v != SymbolTable::Null)
					return rmt_v.asString();
			}
			return "";
		});
	properties->addVariable<std::string>(
		"Connection",
		[&, this](std::string value) {
			if (remote)
				remote->setGroup(value);
			else
				setConnection(value);
		},
		[&]() -> std::string { return remote ? remote->group() : getConnection(); });
	properties->addVariable<std::string>(
		"Visibility",
		[&, this](std::string value) {
			LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(value);
			if (visibility)
				visibility->unlink(this);
			visibility = lp;
			if (lp)
				lp->link(new LinkableVisibility(this));
		},
		[&]() -> std::string { return visibility ? visibility->tagName() : ""; });
}
