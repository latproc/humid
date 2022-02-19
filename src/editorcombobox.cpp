//
//  EditorComboBox.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include "editorcombobox.h"
#include "colourhelper.h"
#include "editor.h"
#include "editorwidget.h"
#include "helper.h"
#include "propertyformhelper.h"
#include <iostream>
#include <nanogui/opengl.h>
#include <nanogui/theme.h>
#include <nanogui/widget.h>

const std::map<std::string, std::string> &EditorComboBox::property_map() const {
    auto structure_class = findClass("COMBOBOX");
    assert(structure_class);
    return structure_class->property_map();
}

const std::map<std::string, std::string> &EditorComboBox::reverse_property_map() const {
    auto structure_class = findClass("COMBOBOX");
    assert(structure_class);
    return structure_class->reverse_property_map();
}

EditorComboBox::EditorComboBox(NamedObject *owner, Widget *parent, const std::string nam,
                               LinkableProperty *lp)
    : ComboBox(parent, {}), EditorWidget(owner, "COMBOBOX", nam, this, lp),
      mBackgroundColor(nanogui::Color(0, 0)), mTextColor(nanogui::Color(0, 0)) {
    setItems({"one", "two", "three"});
}

EditorComboBox::~EditorComboBox() {}

bool EditorComboBox::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down,
                                      int modifiers) {

    using namespace nanogui;

    if (editorMouseButtonEvent(this, p, button, down, modifiers))
        return ComboBox::mouseButtonEvent(p, button, down, modifiers);

    return true;
}

bool EditorComboBox::mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel,
                                      int button, int modifiers) {

    if (editorMouseMotionEvent(this, p, rel, button, modifiers))
        return ComboBox::mouseMotionEvent(p, rel, button, modifiers);

    return true;
}

bool EditorComboBox::mouseEnterEvent(const Vector2i &p, bool enter) {

    if (editorMouseEnterEvent(this, p, enter))
        return ComboBox::mouseEnterEvent(p, enter);

    return true;
}

void EditorComboBox::draw(NVGcontext *ctx) {
    ComboBox::draw(ctx);
    NVGcolor textColor = mTextColor;

    if (mBackgroundColor != nanogui::Color(0, 0)) {
        nvgBeginPath(ctx);
        if (border == 0)
            nvgRect(ctx, mPos.x() + 1, mPos.y() + 1.0f, mSize.x() - 2, mSize.y() - 2);
        else {
            int a = border / 2 + 1;
            nvgRoundedRect(ctx, mPos.x() + a, mPos.y() + a, mSize.x() - 2 * a, mSize.y() - 2 * a,
                           mTheme->mButtonCornerRadius);
        }
        nvgFillColor(ctx, nanogui::Color(mBackgroundColor));
        nvgFill(ctx);
    }

    if (border > 0) {
        nvgBeginPath(ctx);
        nvgStrokeWidth(ctx, border);
        int a = border / 2;
        nvgRoundedRect(ctx, mPos.x() + a, mPos.y() + a, mSize.x() - 2 * a, mSize.y() - 2 * a,
                       mTheme->mButtonCornerRadius);
        nvgStrokeColor(ctx, mTheme->mBorderMedium);
        nvgStroke(ctx);
    }

    if (mSelected)
        drawSelectionBorder(ctx, mPos, mSize);
    else if (EDITOR->isEditMode()) {
        drawElementBorder(ctx, mPos, mSize);
    }
}

void EditorComboBox::loadPropertyToStructureMap(std::map<std::string, std::string> &properties) {
    properties = property_map();
}

void EditorComboBox::getPropertyNames(std::list<std::string> &names) {
    EditorWidget::getPropertyNames(names);
    names.push_back("Background Colour");
    names.push_back("Text Colour");
}

Value EditorComboBox::getPropertyValue(const std::string &prop) {
    Value res = EditorWidget::getPropertyValue(prop);
    if (res != SymbolTable::Null)
        return res;
    if (prop == "Background Colour" && backgroundColor() != mTheme->mTransparent) {
        return Value(stringFromColour(backgroundColor()), Value::t_string);
    }
    if (prop == "Text Colour") {
        nanogui::Widget *w = dynamic_cast<nanogui::Widget *>(this);
        nanogui::Label *lbl = dynamic_cast<nanogui::Label *>(this);
        return Value(stringFromColour(mTextColor), Value::t_string);
    }

    return SymbolTable::Null;
}

void EditorComboBox::setProperty(const std::string &prop, const std::string value) {
    EditorWidget::setProperty(prop, value);
    if (prop == "Remote") {
        if (remote) {
            remote->link(new LinkableText(this));
        }
    }
    if (prop == "Background Colour") {
        getDefinition()->getProperties().add("bg_color", value);
        setBackgroundColor(colourFromProperty(getDefinition(), "bg_color"));
    }
    if (prop == "Text Colour") {
        getDefinition()->getProperties().add("text_colour", value);
        setTextColor(colourFromProperty(getDefinition(), "text_colour"));
    }
    if (prop == "Vertical Pos" || prop == "Horizontal Pos") {
        auto pos = mPos;
        pos.x() += mSize.x();
        assert(popup());
        popup()->setAnchorPos(pos);
    }
}

void EditorComboBox::loadProperties(PropertyFormHelper *properties) {
    EditorWidget::loadProperties(properties);
    EditorComboBox *lbl = dynamic_cast<EditorComboBox *>(this);
    nanogui::Widget *w = dynamic_cast<nanogui::Widget *>(this);
    if (w) {
        properties->addVariable<nanogui::Color>(
            "Text Colour",
            [&, lbl](const nanogui::Color &value) mutable { lbl->setTextColor(value); },
            [&, lbl]() -> const nanogui::Color & { return lbl->textColor(); });
        properties->addVariable<nanogui::Color>(
            "Background Colour",
            [&, lbl](const nanogui::Color &value) mutable { lbl->setBackgroundColor(value); },
            [&, lbl]() -> const nanogui::Color & { return lbl->backgroundColor(); });
        properties->addGroup("Remote");
        properties->addVariable<std::string>(
            "Remote object",
            [&, this, properties](std::string value) {
                LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(value);
                this->setRemoteName(value);
                if (remote)
                    remote->unlink(this);
                remote = lp;
                if (lp) {
                    lp->link(new LinkableText(this));
                }
                //properties->refresh();
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
            [&, this, properties](std::string value) {
                if (remote)
                    remote->setGroup(value);
                else
                    setConnection(value);
            },
            [&]() -> std::string { return remote ? remote->group() : getConnection(); });
        properties->addVariable<std::string>(
            "Visibility",
            [&, this, properties](std::string value) {
                LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(value);
                if (visibility)
                    visibility->unlink(this);
                visibility = lp;
                if (lp) {
                    lp->link(new LinkableVisibility(this));
                }
            },
            [&]() -> std::string { return visibility ? visibility->tagName() : ""; });
    }
}
