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
    using namespace nanogui;
    Widget::draw(ctx);
    nvgEndFrame(ctx); // Flush the NanoVG draw stack, not necessary to call nvgBeginFrame afterwards.

    if (border) drawImageBorder(ctx);

    // Calculate several variables that need to be send to OpenGL in order for the image to be
    // properly displayed inside the widget.
    const Screen* screen = dynamic_cast<const Screen*>(this->window()->parent());
    assert(screen);
    Vector2f screenSize = screen->size().cast<float>();
    Vector2f scaleFactor = mScale * imageSizeF().cwiseQuotient(screenSize);
    Vector2f positionInScreen = absolutePosition().cast<float>();
    Vector2f positionAfterOffset = positionInScreen + mOffset;
    Vector2f imagePosition = positionAfterOffset.cwiseQuotient(screenSize);
    glEnable(GL_SCISSOR_TEST);
    float r = screen->pixelRatio();
    glScissor(positionInScreen.x() * r,
              (screenSize.y() - positionInScreen.y() - size().y()) * r,
              size().x() * r, size().y() * r);
    mShader.bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mImageID);
    mShader.setUniform("image", 0);
    mShader.setUniform("scaleFactor", scaleFactor);
    mShader.setUniform("position", imagePosition);
    mShader.drawIndexed(GL_TRIANGLES, 0, 2);
    glDisable(GL_SCISSOR_TEST);

    if (helpersVisible())
        drawHelpers(ctx);

    if (border) drawWidgetBorder(ctx);
    if (mSelected) drawSelectionBorder(ctx, mPos, mSize);
}

void EditorImageView::setImageName(const std::string new_name, bool reload) {
    image_name = new_name;
    GLuint img = EDITOR->gui()->getImageId(new_name.c_str(), reload);
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
    return Value(imageName(), Value::t_string);
  else if (prop == "Scale")
    return scale();
  return SymbolTable::Null;
}

void EditorImageView::setProperty(const std::string &prop, const std::string value) {
  EditorWidget::setProperty(prop, value);
  if (prop == "Image File") {
    setImageName(value);
    fit();
  }
  else if (prop == "Remote") {
    if (remote) remote->unlink(this);
    remote = EDITOR->gui()->findLinkableProperty(value);
    if (remote) {
        remote->link(new LinkableText(this));
    }
  }
  else if (prop == "Scale") {
    setScale(std::atof(value.c_str()));
  }
}

