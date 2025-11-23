#include "colourhelper.h"
#include "editor.h"
#include "linkmanager.h"
#include "propertyformhelper.h"
#include <helper.h>
#include <nanogui/widget.h>

#include <iostream>

// Helper to remove all children from a widget
static void clearChildren(NamedObject *parent, nanogui::Widget *w, const std::string &prefix) {
    if (parent) {
        int count = 0;
        for (auto iter = parent->locals().begin(); iter != parent->locals().end(); ) {
            auto name = iter->first;
            auto widget =  dynamic_cast<nanogui::Widget*>(iter->second);
            auto curr = iter++;
            if (name.substr(0, prefix.size()) == prefix) {
                ++count;
                parent->locals().erase(curr);
                if (widget) {
                    w->removeChild(widget);
                }
            }
        }
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

    mHeaderSpec = cJSON_CreateArray();
    mData = cJSON_CreateArray();
#if 0
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
    // row as a plain string
    cJSON_AddItemToArray(mData, cJSON_CreateString("This is a plain string row"));
    assert(mHeaderSpec);
    assert(cJSON_IsArray(mHeaderSpec));
    cJSON_Delete(mHeaderSpec);
    mHeaderSpec = cJSON_Parse(R"([{"field":"Name","label":"Name", "width":40},{"field":"Value","label":"Value", "width":30}])");
#endif
    mScroll = new nanogui::VScrollPanel(this);
    mContainer = new nanogui::Widget(mScroll);
    mContainer->setLayout(new nanogui::BoxLayout(
        nanogui::Orientation::Vertical, nanogui::Alignment::Fill, 0, 2));
    rebuildHeader();
    rebuild();
}

EditorTable::~EditorTable() {
    if (mHeaderSpec) { cJSON_Delete(mHeaderSpec); }
    if (mData) { cJSON_Delete(mData); }
    clearChildren(this, mContainer, "");
}

void EditorTable::rebuildHeader() {
    extern std::string table_header_font;
    header = find_or_create_label("header", "");
    assert(header);
    if (mRows.size() == 0) {
        mRows.push_back(header);
    }
    else {
        mRows[0] = header;
    }
    header->setBackgroundColor(nanogui::Color(220,220,0,0));
    header->setPropertyValue("Alignment", "0");
    header->setPropertyValue("Vertical Alignment", "1");
    header->setBorder(border);
    header->setFont(table_header_font);
    header->setFontSize(fontSize());

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
    if (mData) { cJSON_Delete(mData); }
    mData = data;
    rebuild();
}

void EditorTable::setHeader(cJSON *header_spec) {
    if (mHeaderSpec) { cJSON_Delete(mHeaderSpec); }
    mHeaderSpec = header_spec;
    rebuildHeader();
}

void EditorTable::clearSelection() {
    if (mSelectedRow == -1) { return; }
    if (mSelectedRow >= 0 && mSelectedRow < (int)mRows.size()) {
        mRows[mSelectedRow]->setBackgroundColor(nanogui::Color(0,0,0,0));
    }
    mSelectedRow = -1;
    update_remote_selection(mSelectedRow);
}

void EditorTable::setSelectedRow(int index) {
    // Ignore index 0 (header)
    if (index <= 0 || index >= (int)mRows.size()) {
        return;
    }

    if (mSelectedRow != index) {
        // Deselect previous row
        if (mSelectedRow >= 0 && mSelectedRow < (int)mRows.size()) {
            mRows[mSelectedRow]->setBackgroundColor(nanogui::Color(0,0,0,0));
        }
        // Select new row
        mSelectedRow = index;
        mRows[mSelectedRow]->setBackgroundColor(nanogui::Color(200,200,255,255));
        update_remote_selection(mSelectedRow);
    }
}

EditorLabel *EditorTable::find_or_create_label(const std::string & name, const std::string & text) {
    EditorLabel *label = nullptr;
    label = nullptr;

    if (auto obj = find(name)) {
        label = dynamic_cast<EditorLabel*>(obj);
        if (!label) {
            std::cerr << "Existing " << name << " is not a label\n";
        }
    }
    if (label) {
        label->setCaption(text.c_str());
        return label;
    }

    label = new EditorLabel(this, mContainer, name, mLinkedOption, text);
    assert(label);
    return label;
}

void EditorTable::rebuild() {
    extern std::string table_font;
    assert(header);
    mContainer->setFixedHeight(height());
    mContainer->setFixedWidth(width());
    mScroll->setFixedHeight(height());
    mScroll->setFixedWidth(width());
    clearChildren(this, mContainer, "row_");
    mRows.clear();
    mRows.push_back(header);
    mSelectedRow = -1;

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
                cJSON *fldname = cJSON_GetObjectItem(col, "field");
                cJSON *width = cJSON_GetObjectItem(col, "width");
                int w = (width && cJSON_IsNumber(width)) ? width->valueint : 10;
                std::string field;
                if (fldname && cJSON_IsString(fldname)) {
                    cJSON *field_val = cJSON_IsObject((item)) ? cJSON_GetObjectItem(item, fldname->valuestring) : cJSON_GetArrayItem(item, column++);
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

        EditorLabel *label = find_or_create_label(std::string("row_") + std::to_string(index), text);
        label->setBackgroundColor(nanogui::Color(0,0,0,0));
        label->setPropertyValue("Alignment", "0");
        label->setPropertyValue("Vertical Alignment", "1");
        label->setBorder(0);
        label->setFont(table_font);
        label->setFontSize(fontSize());
        label->setFixedWidth(width());

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
            remote->link(new LinkableJson(this));
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
        if (lp) { lp->link(new LinkableJson(this)); }
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

void EditorTable::update_remote_selection(int index) {
    auto select_row_remote = [&](int index) {
        // Store zero-based data row (excluding header) in mLinkedOption if present
        if (!getDefinition()) { return; }
        if (!getParent()) {
            std::cout << "No parent for " << getName() << " when selecting row" << std::endl;
            return;
        }
        assert(getDefinition());
        assert(getParent());
        auto *structure = dynamic_cast<Structure*>(getParent());
        if (!structure) {
            std::cout << "No structure for " << getName() << " when selecting row" << std::endl;
        }
        assert(structure);
        auto & kind = structure->getStructureDefinition()->getName();
        auto remote_links = LinkManager::instance().remote_links(kind, this->EditorWidget::getName());
        if (remote_links) for (auto & link_info : *remote_links) {
            if (link_info.property_name != "selected_row") {
                continue;
            }
            auto linkable_property = EDITOR->gui()->findLinkableProperty(link_info.remote_name);
            if (linkable_property) {
                const std::string &conn = getRemote()->group();
                EDITOR->gui()->queueMessage(conn,
                                            EDITOR->gui()->getIODSyncCommand(conn, getRemote()->getKind(), linkable_property->address(), index), [](std::string s) {
                                            });
                break;
            }
        }

    };
    select_row_remote(index);
}

bool EditorTable::mouseButtonEvent(const nanogui::Vector2i &mouse_pos, int button, bool down, int modifiers) {

    using namespace nanogui;

    if (!editorMouseButtonEvent(this, mouse_pos, button, down, modifiers)) {
        return false;
    }
    Vector2i top_left = mouse_pos - mPos;

    if (button == GLFW_MOUSE_BUTTON_LEFT && down) {
        bool found = false;

        float scroll_offset = 0.0f;
        if (mRows.size() > 1) {
            size_t scrollable_pixels = mRows.size() * (mRows[1]->size().y() + 2) - size().y(); // labels have padding
            scroll_offset = scrollable_pixels * mScroll->scroll();
       }

        for (size_t i = 1; i < mRows.size(); ++i) {
            const auto *label = mRows[i];
            nanogui::Vector2i pos = label->position() - Vector2i(0, std::round(scroll_offset)+1);
            nanogui::Vector2i size = label->size() + Vector2i(0, 2); // padding compensation
            if (top_left.x() >= pos.x() && top_left.x() <= pos.x() + size.x() &&
                top_left.y() >= pos.y() && top_left.y() <= pos.y() + size.y()) {
                setSelectedRow((int)i);
                found = true;
                break;
            }
        }
        if (!found) {
            setSelectedRow(-1);
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
    mContainer->setPosition(Vector2i(1,1));
    mContainer->performLayout(ctx);
    nanogui::Widget::draw(ctx);
    nvgStrokeWidth(ctx, border);
    nvgBeginPath(ctx);
    nvgRect(ctx, mPos.x(), mPos.y(), mSize.x(), mSize.y());
    nvgStrokeColor(ctx, nvgRGBA(0, 0, 0, 255));
    nvgStroke(ctx);

    if (mSelected)
        drawSelectionBorder(ctx, mPos, mSize);
    else if (EDITOR->isEditMode()) {
        drawElementBorder(ctx, mPos, mSize);
    }
}
