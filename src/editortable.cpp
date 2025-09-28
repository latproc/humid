#include <nanogui/widget.h>
#include <helper.h>
#include "propertyformhelper.h"
#include "colourhelper.h"
#include "editor.h"

#include <iostream>

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

    mData = cJSON_CreateArray();
    // Example object rows matching the header spec
    {
        cJSON *row = cJSON_CreateObject();
        cJSON_AddStringToObject(row, "Name", "Fred");
        cJSON_AddStringToObject(row, "Value", "42");
        cJSON_AddItemToArray(mData, row);
    }

    {
        cJSON *row = cJSON_CreateObject();
        cJSON_AddStringToObject(row, "Name", "Sally");
        cJSON_AddStringToObject(row, "Value", "123");
        cJSON_AddItemToArray(mData, row);
    }

    {
        cJSON *row = cJSON_CreateArray();
        cJSON_AddItemToArray(row, cJSON_CreateString("Alice"));
        cJSON_AddItemToArray(row, cJSON_CreateString("99"));
        cJSON_AddItemToArray(mData, row);
    }

    // Special case: row as a plain string
    cJSON_AddItemToArray(mData, cJSON_CreateString("This is a plain string row"));
    mHeaderSpec = cJSON_Parse("[{\"label\":\"Name\", \"width\":40},{\"label\":\"Value\", \"width\":30}]");
    auto str = cJSON_Print(mHeaderSpec);
    std::cout << str << std::endl;
    free(str);
    assert(mHeaderSpec);
    assert(cJSON_IsArray(mHeaderSpec));
    mScroll = new nanogui::VScrollPanel(this);
    mContainer = new nanogui::Widget(mScroll);
    mContainer->setLayout(new nanogui::BoxLayout(
        nanogui::Orientation::Vertical, nanogui::Alignment::Fill, 0, 2));
    rebuildHeader();
    rebuild();
}

void EditorTable::rebuildHeader() {
    extern std::string table_header_font;
    if (!header) {
        header = new EditorLabel(getParent(), mContainer,
                       "header",
                       mLinkedOption, "TEST");
        header->setBackgroundColor(nanogui::Color(220,220,0,0));
        header->setPropertyValue("Alignment", "0");
        header->setPropertyValue("Vertical Alignment", "1");
        header->setBorder(1);
        header->setFont(table_header_font);
    }

    // Build header text from mHeaderSpec
    std::string headerText;
    assert(mHeaderSpec);
    assert(cJSON_IsArray(mHeaderSpec));

    if (mHeaderSpec && cJSON_IsArray(mHeaderSpec)) {
        for (cJSON *item = mHeaderSpec->child; item; item = item->next) {
            if (cJSON_IsObject(item)) {
                cJSON *label = cJSON_GetObjectItem(item, "label");
                cJSON *width = cJSON_GetObjectItem(item, "width");
                if (label && cJSON_IsString(label) && width && cJSON_IsNumber(width)) {
                    std::string lbl = label->valuestring;
                    int w = width->valueint;
                    if ((int)lbl.size() < w) {
                        lbl.append(w - lbl.size(), ' ');
                    } else if ((int)lbl.size() > w) {
                        lbl = lbl.substr(0, w);
                    }
                    headerText += lbl + " ";
                }
            }
        }
    } else {
        headerText = "Header Row";
    }
    header->setCaption(headerText);
}

void EditorTable::setData(cJSON *data) {
    mData = data;
    rebuild();
}

void EditorTable::clearSelection() {
    if (mSelectedRow == -1) { return; }
    if (mSelectedRow >= 0 && mSelectedRow < (int)mRows.size()) {
        mRows[mSelectedRow]->setBackgroundColor(nanogui::Color(0,0,0,0));
    }
    mSelectedRow = -1;
}

void EditorTable::setSelectedRow(int index) {
    // Ignore index 0 (header)
    if (index == 0 || index < 0 || index >= (int)mRows.size())
        return;

    // If clicked row is already selected, deselect it
    if (mSelectedRow == index) {
        if (mSelectedRow >= 0 && mSelectedRow < (int)mRows.size()) {
            mRows[mSelectedRow]->setBackgroundColor(nanogui::Color(0,0,0,0));
        }
        mSelectedRow = -1;
        if (mLinkedOption)
            mLinkedOption->setValue(-1);
        return;
    }

    // Deselect previous row if valid
    if (mSelectedRow >= 0 && mSelectedRow < (int)mRows.size()) {
        mRows[mSelectedRow]->setBackgroundColor(nanogui::Color(0,0,0,0));
    }

    // Select new row
    mSelectedRow = index;
    mRows[mSelectedRow]->setBackgroundColor(nanogui::Color(200,200,255,255));

    // Store zero-based data row (excluding header) in mLinkedOption if present
    if (mLinkedOption)
        mLinkedOption->setValue(mSelectedRow - 1);
}

void EditorTable::rebuild() {
    extern std::string table_font;
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
            // If item is a string, use its value directly
            text = item->valuestring;
        } else if ((cJSON_IsObject(item) || cJSON_IsArray(item)) && mHeaderSpec && cJSON_IsArray(mHeaderSpec)) {
            // If item is an object, build row string from mHeaderSpec
            // It item is an array, build the row string from elements of the array
            int column = 0;
            for (cJSON *col = mHeaderSpec->child; col; col = col->next) {
                assert(cJSON_IsObject(col));
                cJSON *label = cJSON_GetObjectItem(col, "label");
                cJSON *width = cJSON_GetObjectItem(col, "width");
                int w = (width && cJSON_IsNumber(width)) ? width->valueint : 10;
                std::string field;
                if (label && cJSON_IsString(label)) {
                    cJSON *field_val = cJSON_IsObject((item)) ? cJSON_GetObjectItem(item, label->valuestring) : cJSON_GetArrayItem(item, column++);
                    if (field_val && cJSON_IsString(field_val) && field_val->valuestring) {
                        field = field_val->valuestring;
                    } else if (field_val) {
                        char *dump = cJSON_PrintUnformatted(field_val);
                        if (dump) {
                            field = dump;
                            free(dump);
                        }
                    } else {
                        field = "";
                    }
                }
                // Pad or truncate to width
                if ((int)field.size() < w) {
                    field.append(w - field.size(), ' ');
                } else if ((int)field.size() > w) {
                    field = field.substr(0, w);
                }
                text += field + " ";

            }
        } else {
            // Fallback: dump the whole item
            char *dump = cJSON_PrintUnformatted(item);
            if (dump) {
                text = dump;
                free(dump);
            }
        }

        auto *label = new EditorLabel(getParent(), mContainer,
                                      "row_" + std::to_string(index),
                                      mLinkedOption, text);
        label->setBackgroundColor(nanogui::Color(0,0,0,0));
        label->setCallback([this, index]() { setSelectedRow(index); });
        label->setPropertyValue("Alignment", "0");
        label->setPropertyValue("Vertical Alignment", "1");
        label->setBorder(0);
        label->setFont(table_font);

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
    names.push_back("Header");
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
    Value res = EditorWidget::getPropertyValue(prop);
    if (res != SymbolTable::Null)
        return res;
    if (prop == "Selected Row") { return Value(mSelectedRow); }
    if (prop == "Font Size") return fontSize();
    if (prop == "Alignment") return alignment;
    if (prop == "Vertical Alignment") return valign;
    if (prop == "Header") {
        if (mHeaderSpec) {
            char *dump = cJSON_PrintUnformatted(mHeaderSpec);
            if (dump) {
                std::string s(dump);
                free(dump);
                return Value(s);
            }
        }
        return Value("Empty");
    }

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
    if (prop == "Header") {
        cJSON *newHeader = cJSON_Parse(value.c_str());
        if (newHeader && cJSON_IsArray(newHeader)) {
            if (mHeaderSpec) cJSON_Delete(mHeaderSpec);
            mHeaderSpec = newHeader;
            rebuildHeader();
        } else if (newHeader) {
            cJSON_Delete(newHeader);
        }
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
    properties->addVariable<int> (
    "Selected Row",
    [&](int value) mutable{ setSelectedRow(value) ; },
    [&]()->int{ return mSelectedRow; });
    properties->addVariable<std::string> (
      "Header",
      [&](std::string value) mutable{ setProperty("Header", value); rebuildHeader(); rebuild(); },
      [&]()->std::string {
          char *header_str = cJSON_PrintUnformatted(mHeaderSpec);
          std::string s;
          if (header_str) {
              s = header_str;
              free(header_str);
          }
          return s;
      });
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

    if (editorMouseButtonEvent(this, p, button, down, modifiers)) {
        return true;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && down) {
        bool found = false;
        for (size_t i = 1; i < mRows.size(); ++i) {
            auto *label = mRows[i];
            nanogui::Vector2i pos = label->absolutePosition();
            nanogui::Vector2i size = label->size();
            if (p.x() >= pos.x() && p.x() <= pos.x() + size.x() &&
                p.y() >= pos.y() && p.y() <= pos.y() + size.y()) {
                setSelectedRow((int)i);
                found = true;
                break;
            }
        }
        if (!found) {
            clearSelection();
        }
    }

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
