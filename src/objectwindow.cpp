//
//  ObjectWindow.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <fstream>
#include <iostream>
#include <locale>
#include <map>
#include <set>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/tabwidget.h>
#include <nanogui/textbox.h>
#include <nanogui/vscrollpanel.h>

#include "factorybuttons.h"
#include "helper.h"
#include "linkableproperty.h"
#include "objectwindow.h"
#include "selectablebutton.h"

extern std::string tag_file_name;

using std::locale;

// ref http://stackoverflow.com/questions/7302996/changing-the-delimiter-for-cin-c

struct comma_is_space : std::ctype<char> {
    comma_is_space() : std::ctype<char>(get_table()) {}

    static mask const *get_table() {
        static mask rc[table_size];
        rc[(int)','] = std::ctype_base::space;
        return &rc[0];
    }
};

void ObjectWindow::rebuildWindow() {
    const std::map<std::string, LinkableProperty *> &properties(gui->getLinkableProperties());
    std::set<std::string> groups;
    for (auto item : properties) {
        groups.insert(item.second->group());
    }
    for (auto group : groups) {
        createTab(group.c_str());
        if (boost::filesystem::is_regular_file(group)) { // load a tag file if given
            std::cout << "tag file found, loading: " << group << " from file\n";
            createPanelPage(group.c_str(), gui->getObjectWindow()->getPaletteContent());
        }
        loadItems(group, search_box->value());
    }
    //gui->refreshData();
}

ObjectWindow::ObjectWindow(EditorGUI *screen, nanogui::Theme *theme, const char *tfn)
    : Skeleton(screen), gui(screen) {
    using namespace nanogui;
    gui = screen;
    if (tfn && *tfn)
        tag_file_name = tfn;
    window->setTheme(theme);
    window->setFixedSize(Vector2i(360, 600));
    window->setSize(Vector2i(360, 600));
    window->setPosition(Vector2i(screen->width() - 360, 48));
    window->setTitle("Objects");
    GridLayout *layout = new GridLayout(Orientation::Vertical, 1, Alignment::Fill, 0, 3);
    window->setLayout(layout);
    items = new Widget(window);
    items->setPosition(Vector2i(1, window->theme()->mWindowHeaderHeight + 1));
    items->setLayout(new BoxLayout(Orientation::Vertical));
    items->setFixedSize(
        Vector2i(window->width(), window->height() - window->theme()->mWindowHeaderHeight));

    search_box = new nanogui::TextBox(items);
    search_box->setFixedSize(Vector2i(200, 25));
    search_box->setPosition(nanogui::Vector2i(5, 5));
    search_box->setValue("");
    search_box->setEnabled(true);
    search_box->setEditable(true);
    search_box->setAlignment(TextBox::Alignment::Left);
    search_box->setCallback([&, this](const std::string &filter) -> bool {
        std::string group = tab_region->tabLabelAt(tab_region->activeTab());
        loadItems(group, search_box->value());
        return true;
    });
    tab_region = items->add<TabWidget>();
    nanogui::Widget *layer = tab_region->createTab("Local");
    layer->setLayout(new GroupLayout());
    layers["Local"] = layer;
    current_layer = layer;
    tab_region->setActiveTab(0);

    if (tag_file_name.length())
        loadTagFile(tag_file_name);
    window->performLayout(screen->nvgContext());
    window->setVisible(false);
}

nanogui::Widget *ObjectWindow::createTab(const std::string tags) {
    using namespace nanogui;
    const int search_height = 36;
    Widget *container = nullptr;
    std::string tab_name = shortName(tags);
    if (tags.length()) {
        auto found = layers.find(tab_name);
        if (found != layers.end())
            nanogui::Widget *container = (*found).second;
        else {
            container = tab_region->createTab(shortName(tab_name));
            container->setLayout(new GroupLayout());
            layers[tab_name] = container;
        }
        current_layer = container;
        VScrollPanel *palette_scroller = new VScrollPanel(container);
        palette_scroller->setPosition(Vector2i(1, search_height + 5));
        palette_scroller->setFixedSize(
            Vector2i(window->width() - 10,
                     window->height() - window->theme()->mWindowHeaderHeight - search_height - 36));
        palette_content = new Widget(palette_scroller);
        palette_content->setFixedSize(
            Vector2i(window->width() - 15,
                     window->height() - window->theme()->mWindowHeaderHeight - 5 - search_height));
        return palette_content;
    }
    else
        return nullptr;
}

void ObjectWindow::loadTagFile(const std::string tags) {
    using namespace nanogui;
    const int search_height = 36;
    nanogui::Widget *palette_content = createTab(tags);
    if (!palette_content)
        return;
    tag_file_name = tags;

    createPanelPage(tag_file_name.c_str(), palette_content);
    GridLayout *palette_layout = new GridLayout(Orientation::Horizontal, 1, Alignment::Fill);
    palette_layout->setSpacing(1);
    palette_layout->setMargin(4);
    palette_layout->setColAlignment(nanogui::Alignment::Fill);
    palette_layout->setRowAlignment(nanogui::Alignment::Fill);
    palette_content->setLayout(palette_layout);

    if (palette_content->childCount() == 0) {
        Widget *cell = new Widget(palette_content);
        cell->setFixedSize(Vector2i(window->width() - 32, 35));
        new Label(cell, "No match");
    }

    window->performLayout(gui->nvgContext());
}

void ObjectWindow::loadItems(const std::string group, const std::string match) {
    using namespace nanogui;
    clearSelections();
    std::cout << "loading object window items for group " << group << "\n";
    Widget *container = layers[group];
    if (!container || container->childCount() == 0) {
        std::cout << "object window has no tab '" << group << "', nothing to do";
        return;
    }
    Widget *scroller = container->childAt(0);
    VScrollPanel *vs = dynamic_cast<VScrollPanel *>(scroller);
    assert(vs);
    if (!scroller || scroller->childCount() == 0) {
        std::cout << "object tab has no content, nothing to do";
        return;
    }
    Widget *palette_content = scroller->childAt(0);
    if (!palette_content)
        return;
    while (palette_content && palette_content->childCount() > 0) {
        palette_content->removeChild(0);
    }

    std::vector<std::string> tokens;
    boost::algorithm::split(tokens, match, boost::is_any_of(", "));

    int n = tokens.size();
    // for (auto s : tokens) { std::cout << s << " "; } std::cout << "\n";

    bool within_group = group.length() > 0;
    for (auto item : gui->getLinkableProperties()) {
        if (within_group && item.second->group() != group) {
            std::cout << "skipping " << item.first << " (group " << item.second->group() << ")";
            continue;
        }
        for (auto nam : tokens) {
            if (n > 1 && nam.length() == 0)
                continue; //skip empty elements in a list
            if (nam.length() == 0 || item.first.find(nam) != std::string::npos) {
                Widget *cell = new Widget(palette_content);
                LinkableProperty *lp = item.second;
                cell->setFixedSize(Vector2i(window->width() - 32, 35));
                SelectableButton *b = new ObjectFactoryButton(gui, "BUTTON", this, cell, lp);
                b->setEnabled(true);
                b->setFixedSize(Vector2i(window->width() - 40, 30));
                break; // only add once
            }
            else
                std::cout << "skipped: " << item.first << "\n";
        }
    }
    if (palette_content->childCount() == 0) {
        Widget *cell = new Widget(palette_content);
        cell->setFixedSize(Vector2i(window->width() - 32, 35));
        new Label(cell, "No match");
    }
    GridLayout *palette_layout = new GridLayout(Orientation::Horizontal, 1, Alignment::Fill);
    palette_layout->setSpacing(1);
    palette_layout->setMargin(4);
    palette_layout->setColAlignment(nanogui::Alignment::Fill);
    palette_layout->setRowAlignment(nanogui::Alignment::Fill);
    palette_content->setLayout(palette_layout);

    window->performLayout(gui->nvgContext());
}

bool ObjectWindow::importModbusInterface(const std::string file_name, std::istream &init,
                                         nanogui::Widget *palette_content,
                                         nanogui::Widget *container) {

    using namespace nanogui;

    if (!init.good())
        return false;

    char buf[200];

    std::istringstream iss;
    std::string group_name = shortName(file_name);

    iss.imbue(locale(iss.getloc(), new comma_is_space));

    unsigned int pallete_row = 0;

    while (init.getline(buf, 200, '\n')) {
        iss.str(buf);
        iss.clear();
        int protocol_id;
        std::string device_name, tag_name, data_type;
        int data_count;
        std::string retentive;
        std::string address_str;
        int array_start;
        int array_end;

        iss >> protocol_id >> device_name >> tag_name >> data_type >> data_count >> retentive >>
            address_str >> array_start >> array_end;
        char kind = address_str[1];
        if (tag_name.length()) {
            LinkableProperty *lp = gui->findLinkableProperty(tag_name);
            if (lp == nullptr)
                lp = new LinkableProperty(group_name, kind, tag_name, address_str, data_type,
                                          data_count);
            else {
                const std::string &old_group(lp->group());
                int old_kind = lp->getKind();
                const std::string &old_addr_str(lp->addressStr());
                const CircularBuffer::DataType old_dt = lp->dataType();
                int old_size = lp->dataSize();
                lp->setGroup(group_name);
                lp->setKind(kind);
                lp->setAddressStr(address_str);
                lp->setDataTypeStr(data_type);
                lp->setDataSize(data_count);
            }
            gui->getUserWindow()->addDataBuffer(tag_name,
                                                CircularBuffer::dataTypeFromString(data_type),
                                                gui->getSampleBufferSize());
            gui->addLinkableProperty(tag_name, lp);
        }
        /*
		cout << kind << " " << tag_name << " "
		<< data_type << " " << data_count << " " << address_str
		<< "\n";
*/
    }
    loadItems(group_name, search_box->value());

    return true;
}

nanogui::Window *ObjectWindow::createPanelPage(const char *filename,
                                               nanogui::Widget *palette_content) {
    using namespace nanogui;

    if (filename && palette_content) {
        std::string modbus_settings(filename);
        std::ifstream init(modbus_settings);

        if (init.good()) {
            importModbusInterface(filename, init, palette_content, window);
        }
    }

    return window;
}
