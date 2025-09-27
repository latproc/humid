#include <nanogui/widget.h>
#include <helper.h>
#include "propertyformhelper.h"
#include "colourhelper.h"
#include "editor.h"

// Helper to remove all children from a widget
static void clearChildren(nanogui::Widget *w) {
    while (w->childCount() > 0) {
        w->removeChild(0);
    }
}

#include "editortable.h"
#include "nanogui/layout.h"

EditorTable::EditorTable(NamedObject *owner,
                         nanogui::Widget *parent,
                         const std::string nam,
                         LinkableProperty *lp,
                         cJSON *data)
    : nanogui::Widget(parent),
      EditorWidget(owner, "TABLE", nam, this, lp),
      mData(data),
      mLinkedOption(lp)
{
    setLayout(new nanogui::BoxLayout(
        nanogui::Orientation::Vertical, nanogui::Alignment::Fill, 0, 0));

    mScroll = new nanogui::VScrollPanel(this);
    mContainer = new nanogui::Widget(mScroll);
    mContainer->setLayout(new nanogui::BoxLayout(
        nanogui::Orientation::Vertical, nanogui::Alignment::Fill, 0, 2));
    if (!header) {
        header = new EditorLabel(getParent(), mContainer,
                       "header",
                       mLinkedOption, "Header Row");
        header->setBackgroundColor(nanogui::Color(220,220,0,0));
        header->setPropertyValue("Alignment", "1");
        header->setPropertyValue("Vertical Alignment", "1");
        header->setBorder(1);
    }

    rebuild();
}

void EditorTable::setData(cJSON *data) {
    mData = data;
    rebuild();
}

void EditorTable::setSelectedRow(int index) {
    if (index < 0 || index >= (int)mRows.size()) return;

    if (mSelectedRow >= 0 && mSelectedRow < (int)mRows.size())
        mRows[mSelectedRow]->setBackgroundColor(nanogui::Color(0,0,0,0));

    mSelectedRow = index;
    mRows[mSelectedRow]->setBackgroundColor(nanogui::Color(200,200,255,255));

    if (mLinkedOption)
        mLinkedOption->setValue(mSelectedRow);
}

void EditorTable::rebuild() {
    assert(header);
    header->incRef(); // retain the header row
    clearChildren(mContainer);
    mRows.clear();
    mContainer->addChild(header);
    mRows.push_back(header);

    if (!mData || !cJSON_IsArray(mData))
        return;

    int index = 0;
    for (cJSON *item = mData->child; item; item = item->next, ++index) {
        std::string text;
        if (cJSON_IsString(item) && item->valuestring) {
            text = item->valuestring;
        } else {
            char *dump = cJSON_PrintUnformatted(item);
            if (dump) {
                text = dump;
                free(dump); // use hooks if you replaced malloc/free
            }
        }

        auto *label = new EditorLabel(getParent(), mContainer,
                                      "row_" + std::to_string(index),
                                      mLinkedOption, text);
        label->setBackgroundColor(nanogui::Color(0,0,0,0));
        label->setCallback([this, index]() { setSelectedRow(index); });

        mRows.push_back(label);
    }
}

// Minimal EditorWidget plumbing
void EditorTable::getPropertyNames(std::list<std::string> &names) {
    EditorWidget::getPropertyNames(names);
    names.push_back("Font Size");
    names.push_back("Vertical Alignment");
    names.push_back("Alignment");

    names.push_back("Selected Row");
}

void EditorTable::loadPropertyToStructureMap(std::map<std::string, std::string> &properties) {
  properties = property_map();
}

const std::map<std::string, std::string> & EditorTable::property_map() const {
    auto structure_class = findClass("TABLE");
    assert(structure_class);
    return structure_class->property_map();
}

const std::map<std::string, std::string> & EditorTable::reverse_property_map() const {
    auto structure_class = findClass("TABLE");
    assert(structure_class);
    return structure_class->reverse_property_map();
}

Value EditorTable::getPropertyValue(const std::string &prop) {
    if (prop == "selectedRow") { return Value(mSelectedRow); }
    Value res = EditorWidget::getPropertyValue(prop);
    if (res != SymbolTable::Null)
        return res;
    if (prop == "Selected Row") { return Value(mSelectedRow); }
    if (prop == "Font Size") return fontSize();
    if (prop == "Alignment") return alignment;
    if (prop == "Vertical Alignment") return valign;

    return SymbolTable::Null;
}

void EditorTable::setProperty(const std::string &prop, const std::string value) {
    EditorWidget::setProperty(prop, value);
    if (prop == "Remote") {
        if (remote) {
            remote->link(new LinkableText(this));
        }
    }
    if (prop == "Font Size") {
        int fs = std::atoi(value.c_str());
        setFontSize(fs);
        if (header) { header->setFontSize(fs); }
    }
    if (prop == "Alignment") {
        alignment = std::atoi(value.c_str());
        if (header) { header->setProperty(prop, value); }
    }
    if (prop == "Vertical Alignment") {
        valign = std::atoi(value.c_str());
        if (header) { header->setProperty(prop, value); }
    }
    if (prop == "Selected Row") {
        int idx = std::stoi(value);
        setSelectedRow(idx);
    }
}

void EditorTable::loadProperties(PropertyFormHelper* properties) {
    EditorWidget::loadProperties(properties);
    EditorTable *table = dynamic_cast<EditorTable*>(this);
    nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
      if (w) {
    properties->addVariable<int> (
      "Alignment",
      [&](int value) mutable{ alignment = value; },
      [&]()->int{ return alignment; });
    properties->addVariable<int> (
      "Vertical Alignment",
      [&](int value) mutable{ valign = value; },
      [&]()->int{ return valign; });
    properties->addGroup("Remote");
    properties->addVariable<std::string> (
      "Remote object",
      [&,this,properties](std::string value) {
        LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(value);
        this->setRemoteName(value);
        if (remote) remote->unlink(this);
        remote = lp;
        if (lp) { lp->link(new LinkableText(this)); }
        //properties->refresh();
      },
      [&]()->std::string{
        if (remote) return remote->tagName();
        if (getDefinition()) {
          const Value &rmt_v = getDefinition()->getValue("remote");
          if (rmt_v != SymbolTable::Null)
            return rmt_v.asString();
        }
        return "";
    });
    properties->addVariable<std::string> (
      "Connection",
      [&,this,properties](std::string value) {
        if (remote) remote->setGroup(value); else setConnection(value);
      },
      [&]()->std::string{ return remote ? remote->group() : getConnection(); });
    properties->addVariable<std::string> (
      "Visibility",
      [&,this,properties](std::string value) {
        LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(value);
        if (visibility) visibility->unlink(this);
        visibility = lp;
        if (lp) { lp->link(new LinkableVisibility(this)); }
      },
    [&]()->std::string{ return visibility ? visibility->tagName() : ""; });
  }

}

bool EditorTable::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) {

    using namespace nanogui;

    if (editorMouseButtonEvent(this, p, button, down, modifiers))
        return true; //nanogui::Label::mouseButtonEvent(p, button, down, modifiers);

    return true;
}

bool EditorTable::mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) {

    if (editorMouseMotionEvent(this, p, rel, button, modifiers))
        return true; //Label::mouseMotionEvent(p, rel, button, modifiers);

    return true;
}

bool EditorTable::mouseEnterEvent(const Vector2i &p, bool enter) {

    if (editorMouseEnterEvent(this, p, enter))
        return true; //Label::mouseEnterEvent(p, enter);

    return true;
}

void EditorTable::draw(NVGcontext *ctx) {
    mContainer->setFixedSize({this->width(), this->height()});
    mContainer->performLayout(ctx);
    nanogui::Widget::draw(ctx);
    nvgBeginPath(ctx);
    nvgStrokeWidth(ctx, 1.0);
    nvgMoveTo(ctx, mPos.x(), mPos.y() + mSize.y());
    nvgLineTo(ctx, mPos.x() + mSize.x(), mPos.y() + mSize.y());
    nvgLineTo(ctx, mPos.x() + mSize.x(), mPos.y());
    nvgLineTo(ctx, mPos.x(), mPos.y());
    if (mSelected)
        drawSelectionBorder(ctx, mPos, mSize);
    else if (EDITOR->isEditMode()) {
        drawElementBorder(ctx, mPos, mSize);
    }
}
