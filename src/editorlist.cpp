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
#include <chrono>
#include "anchor.h"

extern int debug;

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

void send_property_updates(const std::string & connection_name, const std::vector<std::pair<const LinkManager::LinkInfo *, Value *>> &links) {
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
                                                    link.second->asString());
        //ss << "PROPERTY " << remote_machine << " " << remote_property << " " << link.second << "";
        EDITOR->gui()->queueMessage(connection_name, msg, [](std::string s) {
            if (debug) {
                std::cout << " Response: " << s << "\n";
            }
        });
        delete msg;
    }
}

}; // namespace

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

class EditorList::Impl {
public:
    using PropertyLinks = std::vector<std::pair<const LinkManager::LinkInfo *, Value *>>;
    Impl(EditorList &owner) : owner(owner), last_selected(std::chrono::steady_clock::now()) {
    }

    void report_selection_change() {
        if (m_links.empty()) {
            const auto *link = find_link(owner.remote_links, "selected");
            if (link) {
                m_links.push_back(std::make_pair(link, &m_selected));
            }
            link = find_link(owner.remote_links, "selected_index");
            if (link) {
                m_links.push_back(std::make_pair(link, &selected_index));
            }
        }
        if (!m_links.empty()) {  send_property_updates(owner.connection_name, m_links); }
    }

    const PropertyLinks & links() const { return m_links; }
    void setItemFilename(const std::string &filename);
    int scroll_pos() {
        return m_scroll_pos.iValue;
    }

    void set_scroll_pos(int pos, int min_pos, int max_pos) {
        if (m_scroll_pos == pos) return;
        if (pos < min_pos || pos > max_pos) {
            if (pos < min_pos) pos = min_pos;
            if (pos > max_pos) pos = max_pos;
            m_scroll_pos = pos;
            if (!owner.connection_name.empty() && owner.remote_links) {
                std::vector<std::pair<const LinkManager::LinkInfo *, Value *>> links;
                const auto *scroll_pos_link = find_link(owner.remote_links, "scroll_pos");
                if (scroll_pos_link) {
                    links.push_back(std::make_pair(scroll_pos_link, &m_scroll_pos));
                    send_property_updates(owner.connection_name, links);
                }
            }
        }
        else { m_scroll_pos = pos; }
    }

    const std::string & selected() { return m_selected.sValue; }

    // whatever the user selects overrides backend changes for a
    // period of time.
    void debounce_select(int index) {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> debounce_duration = now - last_selected;
        if (debounce_duration > std::chrono::duration<double>(0.200)) {
            last_selected = now;
            pending_index = index;
        }
    }

    int debounced_index() {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> debounce_duration = now - last_selected;
        if (debounce_duration > std::chrono::duration<double>(0.200)) {
            pending_index = -1;
        }
        return pending_index;
    }

    std::vector<std::string> mItems;
    std::string m_item_str;
    std::string m_item_file;
    Value m_selected = Value("", Value::t_string);
    Value selected_index = -1;
#ifdef linux
    time_t last_file_mod = 0;
#else
    timespec last_file_mod = {0, 0};
#endif
    char item_delimiter = ';';

private:
    EditorList &owner;
    PropertyLinks m_links;
    Value m_scroll_pos = 0;
    std::chrono::steady_clock::time_point last_selected;
    int pending_index = -1;
};

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
    impl = new Impl(*this);
}

EditorList::~EditorList() {
    delete impl;
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
    if (impl->m_item_file.empty()) {
        loadItems(impl->m_item_str);
        return;
    }
    struct stat file_stat;
    int err = stat(impl->m_item_file.c_str(), &file_stat);
    if (err == -1) {
        return; // the file was tested and error logged when set
    }
#ifdef linux
    if (impl->last_file_mod < file_stat.st_mtime) {
#else
    if (impl->last_file_mod < file_stat.st_mtimespec) {
#endif
        char contents[file_stat.st_size + 1];
        std::ifstream in(impl->m_item_file);
        in.read(contents, file_stat.st_size);
        contents[file_stat.st_size] = 0;
        in.close();
        loadItems(contents);
    }
}

void EditorList::reportSelectionChange() {
    if (!connection_name.empty() && remote_links) {
        impl->report_selection_change();
    }
}

void EditorList::performLayout(NVGcontext *ctx) {
    using namespace nanogui;
    clearSelections();
    for (int i = palette_content->childCount(); i > 0;) {
        palette_content->removeChild(--i);
    }
    for (const auto &item : impl->mItems) {
        Widget *cell = new Widget(palette_content);
        cell->setFixedSize(Vector2i(width() - 10, 30));
        SelectableButton *b = new ListItemButton(EDITOR->gui(), this, item, cell);
        b->setCaption(item);
        b->setEnabled(true);
        b->setFixedSize(Vector2i(width() - 10, 29));
        b->setTheme(EDITOR->gui()->getTheme());
        b->setPassThrough(true);
        b->setCallback([this, item]() { setUserSelected(item); });
    }
    int content_height = impl->mItems.size() * 30;
    palette_scroller->setFixedSize(Vector2i(width(), height()));
    palette_content->setFixedSize(Vector2i(width() - 5, content_height));
    Widget::performLayout(ctx);
    if (!impl->mItems.empty()) {
        if (!impl->selected().empty()) {
            setSelected(impl->selected());
        }
        else {
            scroll_to(impl->scroll_pos());
        }
    }
}

void EditorList::loadItems(const std::string &str) {
    impl->mItems.clear();
    size_t start = 0;
    while (start != std::string::npos) {
        auto end = str.find(impl->item_delimiter, start);
        if (end == std::string::npos) {
            auto item = str.substr(start);
            if (!item.empty()) {
                impl->mItems.push_back(item);
            }
            start = end;
        }
        else {
            impl->mItems.push_back(str.substr(start, end - start));
            start = ++end;
        }
    }
    performLayout(EDITOR->gui()->nvgContext());
}

void EditorList::setItems(const std::string &str) {
    impl->m_item_str = str;
    impl->item_delimiter = ';';
    loadItems(impl->m_item_str);
}

void EditorList::Impl::setItemFilename(const std::string &filename) {
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
}

void EditorList::setItemFilename(const std::string &filename) {
    impl->setItemFilename(filename);
    loadItems();
}

const std::string &EditorList::items_str() { return impl->m_item_str; }

const std::string &EditorList::item_filename() { return impl->m_item_file; }

int EditorList::selectedIndex() const {
    long index;
    bool extracted_int = impl->selected_index.asInteger(index);
    assert(extracted_int);
    return index;
}

void EditorList::selectByIndex(int index) {
    if (index < 0 || index >= impl->mItems.size()) {
        impl->selected_index = -1;
        impl->m_selected = Value("", Value::t_string);
        impl->debounce_select(-1);
        clearSelections();
        reportSelectionChange();
        return;
    }
    int last_selected_index = impl->debounced_index();
    if (last_selected_index != -1 && last_selected_index != index) {
        std::cout << "ignoring request to select " << index << "\n";
        return;
    }
    impl->debounce_select(index);

    auto widget = palette_content->childAt(index);
    if (widget) {
        if (widget->childCount() == 1) {
            SelectableButton *sel = dynamic_cast<SelectableButton*>(widget->childAt(0));
            if (sel) {
                sel->select();
                impl->selected_index = index;
                impl->m_selected = Value(impl->mItems[index], Value::t_string);
                if (!is_visible(index)) { scroll_to(index); }
                reportSelectionChange();
                return;
            }
        }
    }
    impl->selected_index = -1;
    impl->m_selected = Value("", Value::t_string);
    reportSelectionChange();
}

const std::string &EditorList::selected() const { return impl->selected(); }

void EditorList::scroll_to(int index) {
    int height = mSize.y();
    int rows = height / 30;
    int max_scroll = impl->mItems.size() - rows;
    if (max_scroll < 0) max_scroll = 0;
    impl->set_scroll_pos(index, 0, max_scroll);
    index = impl->scroll_pos();
    palette_scroller->setScroll(max_scroll == 0 ? 0.0f : 1.0f * index / max_scroll);
}

bool EditorList::is_visible(int index) {
    if (index < 0) { return true; }
    int height = mSize.y();
    int rows = height / 30;
    int max_scroll = impl->mItems.size() - rows;
    if (index > max_scroll) index = max_scroll;
    int current = (palette_scroller->scroll() * max_scroll);
    return index >= current && index <= current + rows;
}

void EditorList::setSelected(const std::string &sel) {
    int i = 0;
    for (const auto &item : impl->mItems) {
        if (item == sel) {
            selectByIndex(i);
            return;
        }
        ++i;
    }
    selectByIndex(-1);
}

void EditorList::setUserSelected(const std::string &sel) {
    int i = 0;
    for (const auto &item : impl->mItems) {
        if (item == sel) {
            impl->debounce_select(i);
            selectByIndex(i);
            return;
        }
        ++i;
    }
    selectByIndex(-1);
}

void EditorList::draw(NVGcontext *ctx) {
    // poll for file changes
    if (!impl->m_item_file.empty()) {
        struct stat file_stat;
        int err = stat(impl->m_item_file.c_str(), &file_stat);
#ifdef linux
        if (err == 0 && impl->last_file_mod < file_stat.st_mtime) {
#else
        if (err == 0 && impl->last_file_mod < file_stat.st_mtimespec) {
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
    names.push_back("Scroll Pos");
}

Value EditorList::getPropertyValue(const std::string &prop) {
    Value res = EditorWidget::getPropertyValue(prop);
    if (res != SymbolTable::Null)
        return res;
    else if (prop == "Items")
        return Value(items_str(), Value::t_string);
    else if (prop == "Items File")
        return Value(impl->m_item_file, Value::t_string);
    else if (prop == "Font Size")
        return fontSize();
    else if (prop == "Text Colour") {
        nanogui::Widget *w = dynamic_cast<nanogui::Widget *>(this);
        nanogui::Label *lbl = dynamic_cast<nanogui::Label *>(this);
        return Value(stringFromColour(mTextColor), Value::t_string);
    }
    else if (prop == "Alignment")
        return alignment;
    else if (prop == "Vertical Alignment")
        return valign;
    else if (prop == "Wrap Text")
        return wrap_text ? 1 : 0;
    else if (prop == "Background Colour" && backgroundColor() != mTheme->mTransparent) {
        return Value(stringFromColour(backgroundColor()), Value::t_string);
    }
    else if (prop == "Scroll Pos") { return impl->scroll_pos(); }
    else if (prop == "Selected")
        return impl->m_selected;
    else if (prop == "Selected Index")
        return impl->selected_index;

    return SymbolTable::Null;
}

void EditorList::setProperty(const std::string &prop, const std::string value) {
    EditorWidget::setProperty(prop, value);
    if (prop == "Items") {
        setItems(value);
    }
    else if (prop == "Items File") {
        setItemFilename(value);
    }
    else if (prop == "Selected") {
        setSelected(value);
    }
    else if (prop == "Selected Index") {
        selectByIndex(std::atoi(value.c_str()));
    }
    else if (prop == "Scroll Pos") {
        scroll_to(std::atoi(value.c_str()));
    }
    else if (prop == "Remote") {
        if (remote) {
            remote->link(new LinkableText(this));
        }
    }
    else if (prop == "Font Size") {
        int fs = std::atoi(value.c_str());
        setFontSize(fs);
    }
    else if (prop == "Alignment") {
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
    else if (prop == "Vertical Alignment") {
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
    else if (prop == "Wrap Text") {
        wrap_text = (value == "1" || value == "true" || value == "TRUE");
    }
    else if (prop == "Text Colour") {
        getDefinition()->getProperties().add("text_colour", value);
        setTextColor(colourFromProperty(getDefinition(), "text_colour"));
    }
    else if (prop == "Background Colour") {
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
            "Selected Index", [&](int value) mutable { selectByIndex(value); },
            [&]() -> int { return selectedIndex(); });
        properties->addVariable<int>(
            "Scroll Pos", [&](int value) mutable { scroll_to(value); },
            [&]() -> int { return impl->scroll_pos(); });
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
