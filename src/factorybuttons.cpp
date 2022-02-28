//
//  FactoryButtons.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include "factorybuttons.h"
#include "editor.h"
#include "editorbutton.h"
#include "editorcombobox.h"
#include "editorframe.h"
#include "editorgui.h"
#include "editorimageview.h"
#include "editorlabel.h"
#include "editorlineplot.h"
#include "editorlist.h"
#include "editorprogressbar.h"
#include "editortextbox.h"
#include "editorwidget.h"
#include "structure.h"
#include "structureswindow.h"
#include <iostream>
#include <symboltable.h>

extern int run_only;
extern int full_screen_mode;

StructureFactoryButton::StructureFactoryButton(EditorGUI *screen, const std::string type_str,
                                               Palette *pal, nanogui::Widget *parent,
                                               int object_type, const std::string &name,
                                               const std::string &addr_str)
    : SelectableButton(type_str, pal, parent, name), gui(screen), kind(object_type), tag_name(name),
      address_str(addr_str) {}

nanogui::Widget *StructureFactoryButton::create(nanogui::Widget *window) const {
    using namespace nanogui;

    StructureClass *sc = findClass(getClass());
    if (!sc) {
        std::cerr << "No widget class " << getClass() << "\n";
    }
    assert(sc);
    Structure *parent = gui->getUserWindow()->structure();
    Structure *s = sc->instantiate(parent);
    assert(s);
    int object_width = (s) ? s->getIntProperty("width", 80) : 80;
    int object_height = (s) ? s->getIntProperty("height", 60) : 60;
    auto generated_name = NamedObject::nextName(parent);
    Widget *result = 0;
    if (sc->getName() == "BUTTON") {
        EditorButton *b = new EditorButton(parent, window, generated_name, nullptr, caption());
        b->setDefinition(s);
        s->setName(b->getName());
        b->setBackgroundColor(Color(200, 30, 30, 200)); // TBD use structure value
        b->setupButtonCallbacks(b->getRemote(), EDITOR->gui());

        result = b;
    }
    if (sc->getName() == "INDICATOR") {
        EditorButton *b = new EditorButton(parent, window, generated_name, nullptr, caption());
        b->setDefinition(s);
        s->setName(b->getName());
        b->setBackgroundColor(Color(200, 200, 30, 200)); // TBD use structure value

        result = b;
    }
    else if (sc->getName() == "LABEL") {
        EditorLabel *eb = new EditorLabel(parent, window, generated_name, nullptr, "untitled");
        eb->setDefinition(s);
        eb->setName(eb->getName());
        if (s)
            s->setName(eb->getName());
        result = eb;
    }
    else if (sc->getName() == "LIST") {
        EditorList *el = new EditorList(parent, window, generated_name, nullptr, "untitled");
        el->setDefinition(s);
        el->setName(el->getName());
        if (s)
            s->setName(el->getName());
        result = el;
    }
    else if (sc->getName() == "TEXT") {
        EditorGUI *gui = EDITOR->gui();
        EditorTextBox *eb = new EditorTextBox(parent, window, generated_name, nullptr);
        eb->setDefinition(s);
        s->setName(eb->getName());
        eb->setEditable(true);
        eb->setCallback([eb, gui](const std::string &value) -> bool {
            const std::string &conn(eb->getRemote()->group());
            if (!eb->getRemote())
                return true;
            char *rest = 0;
            {
                long val = strtol(value.c_str(), &rest, 10);
                if (*rest == 0) {
                    gui->queueMessage(
                        conn, gui->getIODSyncCommand(conn, 4, eb->getRemote()->address(), (int)val),
                        [](std::string s) { std::cout << s << "\n"; });
                    return true;
                }
            }
            {
                double val = strtod(value.c_str(), &rest);
                if (*rest == 0) {
                    gui->queueMessage(
                        conn,
                        gui->getIODSyncCommand(conn, 4, eb->getRemote()->address(), (float)val),
                        [](std::string s) { std::cout << s << "\n"; });
                    return true;
                }
            }
            return false;
        });
        result = eb;
    }
    else if (sc->getName() == "IMAGE") {
        GLuint img = gui->getImageId("images/blank.png");
        EditorImageView *iv = new EditorImageView(parent, window, generated_name, nullptr, img);
        iv->setDefinition(s);
        s->setName(iv->getName());
        iv->setGridThreshold(20);
        iv->setPixelInfoThreshold(20);
        iv->setImageName("images/blank.png");
        iv->fit();
        result = iv;
    }
    else if (sc->getName() == "PLOT") {
        EditorLinePlot *lp = new EditorLinePlot(parent, window, generated_name, nullptr);
        lp->setDefinition(s);
        s->setName(lp->getName());
        lp->setBufferSize(gui->sampleBufferSize());
        result = lp;
    }
    else if (sc->getName() == "FRAME") {
        auto *fr = new EditorFrame(parent, window, generated_name, nullptr);
        fr->setDefinition(s);
        s->setName(fr->getName());
        result = fr;
    }
    else if (sc->getName() == "PROGRESS") {
        EditorProgressBar *ep = new EditorProgressBar(parent, window, generated_name, nullptr);
        s->setName(ep->getName());
        ep->setDefinition(s);
        result = ep;
    }
    else if (sc->getName() == "COMBOBOX") {
        EditorComboBox *ep = new EditorComboBox(parent, window, generated_name, nullptr);
        s->setName(ep->getName());
        ep->setDefinition(s);
        result = ep;
    }
    if (result) {
        result->setFixedSize(Vector2i(object_width, object_height));
        result->setSize(Vector2i(object_width, object_height));
    }
    return result;
}

ObjectFactoryButton::ObjectFactoryButton(EditorGUI *screen, const std::string type_str,
                                         Palette *pal, nanogui::Widget *parent,
                                         LinkableProperty *lp)
    : SelectableButton(type_str, pal, parent, lp->tagName()), gui(screen), properties(lp) {}

nanogui::Widget *ObjectFactoryButton::create(nanogui::Widget *container) const {
    using namespace nanogui;
    Widget *result = 0;
    int object_width = 60;
    int object_height = 25;

    int kind = properties->getKind();
    const std::string &address_str(properties->addressStr());
    const std::string &tag_name(properties->tagName());

    if (kind == 0 && address_str.length() > 2) {
        kind = address_str[0];
        if (!isdigit(kind))
            kind = address_str[1];
    }

    Structure *parent = gui->getUserWindow()->structure();
    assert(parent);

    switch (kind) {

    case '0': {
        size_t p = tag_name.find(".cmd_");

        if (p != std::string::npos) {
            StructureClass *sc = findClass("BUTTON");
            assert(sc);
            Structure *s = sc->instantiate(gui->getUserWindow()->structure());
            assert(s);

            char *rest = 0;
            int address_int = (int)strtol(address_str.c_str() + 2, &rest, 10);

            if (*rest) {
                std::cerr << "Unexpected data in address\n";
                address_int = 0;
            }

            EditorButton *b = new EditorButton(parent, container, NamedObject::nextName(parent),
                                               properties, tag_name.substr(p + 5), false);
            b->setSize(Vector2i(object_width, object_height));
            b->setDefinition(s);
            b->setFlags(0);
            LinkableProperty *lp = gui->findLinkableProperty(tag_name);
            if (lp)
                b->setRemote(lp);
            b->setupButtonCallbacks(lp, gui);
            result = b;
        }
        else {
            StructureClass *sc = findClass("INDICATOR");
            assert(sc);
            Structure *s = sc->instantiate(gui->getUserWindow()->structure());
            assert(s);
            EditorButton *b =
                new EditorButton(parent, container, NamedObject::nextName(parent), properties);
            b->setSize(Vector2i(object_width, object_height));
            b->setDefinition(s);
            b->setFlags(Button::ToggleButton);
            LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(tag_name);
            if (lp)
                lp->link(new LinkableIndicator(b));
            b->setupButtonCallbacks(lp, gui);
            result = b;
        }
    }

    break;

    case '1': {
        char *rest = 0;
        StructureClass *sc = findClass("INDICATOR");
        assert(sc);
        Structure *s = sc->instantiate(gui->getUserWindow()->structure());
        assert(s);
        EditorButton *b = new EditorButton(parent, container, NamedObject::nextName(parent),
                                           properties, tag_name, true);
        b->setDefinition(s);
        b->setSize(Vector2i(object_width, object_height));
        b->setFixedSize(Vector2i(object_width, object_height));
        b->setFlags(0);
        LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(tag_name);
        if (lp)
            lp->link(new LinkableIndicator(b));
        b->setupButtonCallbacks(lp, gui);
        result = b;
    }

    break;

    case '3': {
        //EditorLabel *el = new EditorLabel(container, nullptr, tag_name, std::string("sans-bold"));
        //el->setSize(Vector2i(object_width, object_height));
        //el->setFixedSize(Vector2i(object_width, object_height));
        //CheckBox *b = new CheckBox(container, tag_name);
        //img->setPolicy(ImageView::SizePolicy::Expand);
        //b->setBackgroundColor(Color(200, 200, 200, 255));
        //b->setFixedSize(Vector2i(25, 25));
        //result = new Label(container, tag_name, "sans-bold");

        EditorLabel *lbl =
            new EditorLabel(parent, container, NamedObject::nextName(parent), properties, "");
        lbl->setName(tag_name);
        lbl->setCaption("");
        lbl->setEnabled(true);
        if (!run_only && !full_screen_mode) {
            lbl->setTooltip(tag_name);
        }
        lbl->setSize(Vector2i(object_width, object_height));
        LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(tag_name);
        if (lp)
            lp->link(new LinkableText(lbl));
        EditorGUI *gui = EDITOR->gui();
        result = lbl;
    }

    break;

    case '4': {
        //new Label(container, tag_name, "sans-bold");
        auto textBox =
            new EditorTextBox(parent, container, NamedObject::nextName(parent), properties);
        textBox->setEditable(true);
        textBox->setSize(Vector2i(object_width, object_height));
        textBox->setFixedSize(Vector2i(object_width, object_height));
        if (!run_only && !full_screen_mode) {
            textBox->setTooltip(tag_name);
        }
        //textBox->setDefaultValue("0");
        //textBox->setFontSize(16);
        //textBox->setFormat("[1-9][0-9]*");
        //textBox->setSpinnable(true);
        //textBox->setMinValue(1);
        //textBox->setValueIncrement(1);
        LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(tag_name);
        if (lp)
            lp->link(new LinkableText(textBox));
        EditorGUI *gui = EDITOR->gui();
        textBox->setCallback([textBox, gui, lp](const std::string &value) -> bool {
            char *rest = 0;
            const std::string &conn(textBox->getRemote()->group());
            {
                long val = strtol(value.c_str(), &rest, 10);
                if (*rest == 0) {
                    gui->queueMessage(
                        conn,
                        gui->getIODSyncCommand(conn, 4, textBox->getRemote()->address(), (int)val),
                        [](std::string s) { std::cout << s << "\n"; });
                    return true;
                }
            }
            {
                double val = strtod(value.c_str(), &rest);
                if (*rest == 0) {
                    gui->queueMessage(conn,
                                      gui->getIODSyncCommand(
                                          conn, 4, textBox->getRemote()->address(), (float)val),
                                      [](std::string s) { std::cout << s << "\n"; });
                    return true;
                }
            }
            if (lp->dataType() == CircularBuffer::STR)
                gui->queueMessage(
                    conn,
                    gui->getIODSyncCommand(conn, 4, textBox->getRemote()->address(), value.c_str()),
                    [](std::string s) { std::cout << s << "\n"; });
            return false;
        });
        result = textBox;
    }

    break;

    default:;
    }
    return result;
}
