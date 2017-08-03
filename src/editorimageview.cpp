//
//  EditorImageView.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include "editor.h"
#include "editorimageview.h"

EditorImageView::EditorImageView(NamedObject *owner, Widget *parent, const std::string nam, LinkableProperty *lp, GLuint image_id, int icon)
: ImageView(parent, image_id), EditorWidget(owner, "IMAGE", nam, this, lp), dh(0), handles(9), handle_coordinates(9,2) {
}

bool EditorImageView::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) {

    using namespace nanogui;

    if (editorMouseButtonEvent(this, p, button, down, modifiers))
        return nanogui::ImageView::mouseButtonEvent(p, button, down, modifiers);

    return true;
}

bool EditorImageView::mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) {

    if (editorMouseMotionEvent(this, p, rel, button, modifiers))
        return ImageView::mouseMotionEvent(p, rel, button, modifiers);

    return true;
}

bool EditorImageView::mouseEnterEvent(const Vector2i &p, bool enter) {

    if (editorMouseEnterEvent(this, p, enter))
        return ImageView::mouseEnterEvent(p, enter);

    return true;
}

void EditorImageView::draw(NVGcontext *ctx) {
    nanogui::ImageView::draw(ctx);
    if (mSelected) drawSelectionBorder(ctx, mPos, mSize);
}

void EditorImageView::setImageName(const std::string new_name) {
    image_name = new_name;
    GLuint img = EDITOR->gui()->getImageId(new_name.c_str(), true);
    mImageID = img;
    updateImageParameters();
}

const std::string &EditorImageView::imageName() const { return image_name; }

void EditorImageView::refresh() {
    need_redraw = true;
}

void EditorImageView::getPropertyNames(std::list<std::string> &names) {
    EditorWidget::getPropertyNames(names);
    names.push_back("Image File");
    names.push_back("Scale");
}

void EditorImageView::loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) {
  EditorWidget::loadPropertyToStructureMap(property_map);
  property_map["Image File"] = "image_file";
  property_map["Scale"] = "scale";
}

Value EditorImageView::getPropertyValue(const std::string &prop) {
  Value res = EditorWidget::getPropertyValue(prop);
  if (res != SymbolTable::Null)
    return res;
  if (prop == "Image File")
    return Value(image_name, Value::t_string);
  else if (prop == "Scale")
    return prop;
  return SymbolTable::Null;
}
