//
//  EditorList.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include "editorlist.h"
#include "colourhelper.h"
#include "editor.h"
#include "editorwidget.h"
#include "helper.h"
#include "propertyformhelper.h"
#include "selectablebutton.h"
#include "valuehelper.h"
#include <fstream>
#include <iostream>
#include <nanogui/layout.h>
#include <nanogui/opengl.h>
#include <nanogui/theme.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/widget.h>
#include <sys/stat.h>
#include <time.h>
#include <utility>

class ListItemButton : public SelectableButton {
  public:
    ListItemButton(EditorGUI *screen, Palette *pal, const std::string label,
                   nanogui::Widget *parent);
    nanogui::Widget *create(nanogui::Widget *container) const override { return nullptr; }

  private:
    EditorGUI *gui;
};

ListItemButton::ListItemButton(EditorGUI *screen, Palette *pal, const std::string label,
                               nanogui::Widget *parent)
    : SelectableButton("BUTTON", pal, parent, label), gui(screen) {}

const std::map<std::string, std::string> &EditorList::property_map() const {
    auto structure_class = findClass("LIST");
    assert(structure_class);
    return structure_class->property_map();
}

const std::map<std::string, std::string> &EditorList::reverse_property_map() const {
    auto structure_class = findClass("LIST");
    assert(structure_class);
    return structure_class->reverse_property_map();
}

EditorList::EditorList(NamedObject *owner, Widget *parent, const std::string nam,
                       LinkableProperty *lp, const std::string caption, const std::string &font,
                       int fontSize, int icon)
    : nanogui::Widget(parent), EditorWidget(owner, "LIST", nam, this, lp),
      mBackgroundColor(nanogui::Color(0, 0)), mTextColor(nanogui::Color(0, 0)),
      Palette(Palette::PT_SINGLE_SELECT), alignment(1), valign(1), wrap_text(true) {
    palette_scroller = new nanogui::VScrollPanel(this);
    palette_scroller->setPosition(Vector2i(0, 0));
    palette_scroller->setFixedSize(Vector2i(width(), height()));
    palette_content = new Widget(palette_scroller);
    palette_content->setLayout(new nanogui::GroupLayout(0, -2, 0, 0));
    palette_content->setFixedSize(Vector2i(width(), height()));
}

bool EditorList::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down,
                                  int modifiers) {

    using namespace nanogui;

    if (editorMouseButtonEvent(this, p, button, down, modifiers)) {
        return nanogui::Widget::mouseButtonEvent(p, button, down, modifiers);
    }

    return true;
}

bool EditorList::mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel,
                                  int button, int modifiers) {

    if (editorMouseMotionEvent(this, p, rel, button, modifiers)) {
        return nanogui::Widget::mouseMotionEvent(p, rel, button, modifiers);
    }

    return true;
}

bool EditorList::mouseEnterEvent(const Vector2i &p, bool enter) {

    if (editorMouseEnterEvent(this, p, enter)) {
        return nanogui::Widget::mouseEnterEvent(p, enter);
    }

    return true;
}

namespace {
bool operator<(const timespec &t1, const timespec &t2) {
    return t1.tv_sec < t2.tv_sec || (t1.tv_sec == t2.tv_sec && t1.tv_nsec < t2.tv_nsec);
}
} // namespace

void EditorList::loadItems() {
    if (m_item_file.empty()) {
        loadItems(m_item_str);
        return;
    }
    struct stat file_stat;
    int err = stat(m_item_file.c_str(), &file_stat);
    if (err == -1) {
        return; // the file was tested and error logged when set
    }
#ifdef linux
    if (last_file_mod < file_stat.st_mtime) {
#else
    if (last_file_mod < file_stat.st_mtimespec) {
#endif
        char contents[file_stat.st_size + 1];
        std::ifstream in(m_item_file);
        in.read(contents, file_stat.st_size);
        contents[file_stat.st_size] = 0;
        in.close();
        loadItems(contents);
    }
}

namespace {
const LinkManager::LinkInfo *find_link(const LinkManager::Links *links,
                                       const std::string &property) {
    if (!links) {
        return nullptr;
    }
    for (const auto &link : *links) {
        if (link.property_name == property)
            return &link;
    }
    return nullptr;
}
}; // namespace

void EditorList::reportSelectionChange() {
    if (!connection_name.empty() && remote_links) {
        std::vector<std::pair<const LinkManager::LinkInfo *, Value>> links;
        const auto *selection_link = find_link(remote_links, "selected");
        const auto *index_link = find_link(remote_links, "selected_index");
        if (selection_link) {
            links.push_back(std::make_pair(selection_link, Value(m_selected, Value::t_string)));
        }
        if (index_link) {
            links.push_back(std::make_pair(index_link, selected_index));
        }
        for (const auto &link : links) {
            std::stringstream ss;
            std::string remote_machine = link.first->remote_name;
            std::string remote_property;
            auto separator_pos = remote_machine.rfind('.');
            if (separator_pos == std::string::npos) {
                remote_property = "VALUE";
            }
            else {
                remote_property = remote_machine.substr(separator_pos + 1);
                remote_machine = remote_machine.substr(0, separator_pos);
            }
            char *msg = MessageEncoding::encodeCommand("PROPERTY", remote_machine, remote_property,
                                                       link.second);
            //ss << "PROPERTY " << remote_machine << " " << remote_property << " " << link.second << "";
            EDITOR->gui()->queueMessage(connection_name, msg, [](std::string s) {
                extern int debug;
                if (debug) {
                    std::cout << " Response: " << s << "\n";
                }
            });
            delete msg;
        }
    }
}

void EditorList::performLayout(NVGcontext *ctx) {
    using namespace nanogui;
    for (int i = palette_content->childCount(); i > 0;) {
        palette_content->removeChild(--i);
    }
    for (const auto &item : mItems) {
        Widget *cell = new Widget(palette_content);
        cell->setFixedSize(Vector2i(width() - 10, 30));
        SelectableButton *b = new ListItemButton(EDITOR->gui(), this, item, cell);
        b->setCaption(item);
        b->setEnabled(true);
        b->setFixedSize(Vector2i(width() - 10, 30));
        b->setTheme(EDITOR->gui()->getTheme());
        b->setPassThrough(true);
        b->setCallback([this, item]() { setSelected(item); });
    }
    int content_height = mItems.size() * 30;
    palette_scroller->setFixedSize(Vector2i(width(), height()));
    palette_content->setFixedSize(
        Vector2i(width() - 5, content_height > height() ? content_height : content_height));
    Widget::performLayout(ctx);
}

void EditorList::loadItems(const std::string &str) {
    mItems.clear();
    size_t start = 0;
    while (start != std::string::npos) {
        auto end = str.find(item_delimiter, start);
        if (end == std::string::npos) {
            auto item = str.substr(start);
            if (!item.empty()) {
                mItems.push_back(item);
            }
            start = end;
        }
        else {
            mItems.push_back(str.substr(start, end - start));
            start = ++end;
        }
    }
    performLayout(EDITOR->gui()->nvgContext());
}

void EditorList::setItems(const std::string &str) {
    m_item_str = str;
    item_delimiter = ';';
    loadItems(m_item_str);
}

void EditorList::setItemFilename(const std::string &filename) {
    if (!filename.empty() && filename == m_item_file) {
#ifdef linux
        last_file_mod = 0; // force an update
#else
        last_file_mod = {0, 0}; // force an update
#endif
        return;
    }
    m_item_file = filename;
    if (m_item_file.empty()) {
        item_delimiter = ';';
    }
    else {
        struct stat file_stat;
        int err = stat(m_item_file.c_str(), &file_stat);
        if (err == -1) {
            std::cerr << "Error: " << strerror(errno) << " looking for file: " << m_item_file
                      << "\n";
        }
        item_delimiter = '\n';
#ifdef linux
        last_file_mod = 0; // force a load
#else
        last_file_mod = {0, 0}; // force a load
#endif
    }
    loadItems();
}

const std::string &EditorList::items_str() { return m_item_str; }

const std::string &EditorList::item_filename() { return m_item_file; }

int EditorList::selectedIndex() const { return selected_index; }

void EditorList::select(int index) {
    if (index < 0 || index >= mItems.size()) {
        selected_index = -1;
        m_selected = "";
        return;
    }
    selected_index = index;
    m_selected = mItems[index];
    reportSelectionChange();
}

const std::string &EditorList::selected() const { return m_selected; }

void EditorList::setSelected(const std::string &sel) {
    int i = 0;
    for (const auto &item : mItems) {
        if (item == sel) {
            select(i);
            return;
        }
        ++i;
    }
    select(-1);
}

void EditorList::draw(NVGcontext *ctx) {
    // poll for file changes
    if (!m_item_file.empty()) {
        struct stat file_stat;
        int err = stat(m_item_file.c_str(), &file_stat);
#ifdef linux
        if (err == 0 && last_file_mod < file_stat.st_mtime) {
#else
        if (err == 0 && last_file_mod < file_stat.st_mtimespec) {
#endif
            // TODO: defer a loadItems()
        }
    }

    Widget::draw(ctx);
    NVGcolor textColor = mTextColor.w() == 0 ? nanogui::Color(0, 255) : mTextColor;

    if (mBackgroundColor != nanogui::Color(0, 0)) {
        nvgBeginPath(ctx);
        if (border == 0)
            nvgRect(ctx, mPos.x() + 1, mPos.y() + 1.0f, mSize.x() - 2, mSize.y() - 2);
        else {
            int a = border / 2;
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

void EditorList::loadPropertyToStructureMap(std::map<std::string, std::string> &properties) {
    properties = property_map();
}

void EditorList::getPropertyNames(std::list<std::string> &names) {
    EditorWidget::getPropertyNames(names);
    names.push_back("Items");
    names.push_back("Items File");
    names.push_back("Font Size");
    names.push_back("Text Colour");
    names.push_back("Vertical Alignment");
    names.push_back("Alignment");
    names.push_back("Wrap Text");
    names.push_back("Background Colour");
    names.push_back("Selected");
    names.push_back("Selected Index");
}

Value EditorList::getPropertyValue(const std::string &prop) {
    Value res = EditorWidget::getPropertyValue(prop);
    if (res != SymbolTable::Null)
        return res;
    if (prop == "Items")
        return Value(items_str(), Value::t_string);
    if (prop == "Items File")
        return Value(m_item_file, Value::t_string);
    if (prop == "Font Size")
        return fontSize();
    if (prop == "Text Colour") {
        nanogui::Widget *w = dynamic_cast<nanogui::Widget *>(this);
        nanogui::Label *lbl = dynamic_cast<nanogui::Label *>(this);
        return Value(stringFromColour(mTextColor), Value::t_string);
    }
    if (prop == "Alignment")
        return alignment;
    if (prop == "Vertical Alignment")
        return valign;
    if (prop == "Wrap Text")
        return wrap_text ? 1 : 0;
    if (prop == "Background Colour" && backgroundColor() != mTheme->mTransparent) {
        return Value(stringFromColour(backgroundColor()), Value::t_string);
    }
    if (prop == "Selected")
        return Value(m_selected, Value::t_string);
    if (prop == "Selected Index")
        return selected_index;

    return SymbolTable::Null;
}

void EditorList::setProperty(const std::string &prop, const std::string value) {
    EditorWidget::setProperty(prop, value);
    if (prop == "Items") {
        setItems(value);
    }
    if (prop == "Items File") {
        setItemFilename(value);
    }
    if (prop == "Selected") {
        setSelected(value);
    }
    if (prop == "Selected Index") {
        select(std::atoi(value.c_str()));
    }
    if (prop == "Remote") {
        if (remote) {
            remote->link(new LinkableText(this));
        }
    }
    if (prop == "Font Size") {
        int fs = std::atoi(value.c_str());
        setFontSize(fs);
    }
    if (prop == "Alignment") {
        long align_int = 0;
        Value val(value);
        if (val.asInteger(align_int)) {
            alignment = static_cast<int>(align_int);
        }
        else {
            if (value == "left") {
                alignment = 0;
            }
            else if (value == "centre" || value == "center") {
                alignment = 1;
            }
            else if (value == "right") {
                alignment = 2;
            }
            else
                alignment = defaultForProperty("alignment").iValue;
        }
    }
    if (prop == "Vertical Alignment") {
        long v_align_int = 0;
        Value val(value);
        if (val.asInteger(v_align_int)) {
            valign = static_cast<int>(v_align_int);
        }
        else {
            if (value == "top") {
                valign = 0;
            }
            else if (value == "centre" || value == "center") {
                valign = 1;
            }
            else if (value == "bottom") {
                valign = 2;
            }
            else
                valign = defaultForProperty("valign").iValue;
        }
    }
    if (prop == "Wrap Text") {
        wrap_text = (value == "1" || value == "true" || value == "TRUE");
    }
    if (prop == "Text Colour") {
        getDefinition()->getProperties().add("text_colour", value);
        setTextColor(colourFromProperty(getDefinition(), "text_colour"));
    }
    if (prop == "Background Colour") {
        getDefinition()->getProperties().add("bg_color", value);
        setBackgroundColor(colourFromProperty(getDefinition(), "bg_color"));
    }
}

void EditorList::loadProperties(PropertyFormHelper *properties) {
    EditorWidget::loadProperties(properties);
    EditorList *lbl = dynamic_cast<EditorList *>(this);
    nanogui::Widget *w = dynamic_cast<nanogui::Widget *>(this);
    if (w) {
        properties->addVariable<std::string>(
            "Items", [&](std::string value) mutable { setItems(value); },
            [&]() -> std::string { return items_str(); });
        properties->addVariable<std::string>(
            "Items File", [&](std::string value) mutable { setItemFilename(value); },
            [&]() -> std::string { return item_filename(); });
        properties->addVariable<std::string>(
            "Selected", [&](std::string value) mutable { setSelected(value); },
            [&]() -> std::string { return selected(); });
        properties->addVariable<int>(
            "Selected Index", [&](int value) mutable { select(value); },
            [&]() -> int { return selectedIndex(); });
        properties->addVariable<int>(
            "Alignment", [&](int value) mutable { alignment = value; },
            [&]() -> int { return alignment; });
        properties->addVariable<int>(
            "Vertical Alignment", [&](int value) mutable { valign = value; },
            [&]() -> int { return valign; });
        properties->addVariable<bool>(
            "Wrap Text", [&](bool value) mutable { wrap_text = value; },
            [&]() -> bool { return wrap_text; });
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
