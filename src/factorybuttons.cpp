//
//  FactoryButtons.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include "structure.h"
#include <symboltable.h>
#include "editor.h"
#include "editorwidget.h"
#include "editorbutton.h"
#include "editortextbox.h"
#include "editorlabel.h"
#include "editorimageview.h"
#include "editorlineplot.h"
#include "editorprogressbar.h"
#include "editorgui.h"
#include "factorybuttons.h"
#include "structureswindow.h"

StructureFactoryButton::StructureFactoryButton(EditorGUI *screen,
						const std::string type_str, Palette *pal,
						nanogui::Widget *parent,
						int object_type,
						const std::string &name, const std::string &addr_str)
	: SelectableButton(type_str, pal, parent, name),
		gui(screen), kind(object_type), tag_name(name), address_str(addr_str) {

}

nanogui::Widget *StructureFactoryButton::create(nanogui::Widget *window) const {
	using namespace nanogui;

	StructureClass *sc = findClass(getClass());
	assert(sc);
	Structure *s = sc->instantiate(gui->getUserWindow()->structure());
	assert(s);
	int object_width = (s) ? s->getIntProperty("width", 80) : 80;
	int object_height = (s) ? s->getIntProperty("height", 60) : 60;
	Widget *result = 0;
	if (sc->getName() == "BUTTON") {
		EditorButton *b = new EditorButton(window, NamedObject::nextName(0), nullptr, caption());
		b->setDefinition(s);
		s->setName(b->getName());
		b->setBackgroundColor(Color(200, 30, 30, 255)); // TBD use structure value
		b->setChangeCallback([b, this] (bool state) {
			if (b->getDefinition()->getKind() == "BUTTON") {
				if (b->getRemote()) {
					gui->queueMessage(
								gui->getIODSyncCommand(0, b->address(), (state)?1:0), [](std::string s){std::cout << s << "\n"; });
					}
				else if (b->command().length()) {
					gui->queueMessage(b->command(), [](std::string s){std::cout << "Response: " << s << "\n"; });
				}
			}
		});

		result = b;
	}
	if (sc->getName() == "INDICATOR") {
		EditorButton *b = new EditorButton(window, NamedObject::nextName(0), nullptr, caption());
		b->setDefinition(s);
		s->setName(b->getName());
		b->setBackgroundColor(Color(200, 200, 30, 200)); // TBD use structure value
		b->setChangeCallback([b, this] (bool state) {
			if (b->getDefinition()->getKind() == "BUTTON") {
				if (b->getRemote()) {
					gui->queueMessage(
								gui->getIODSyncCommand(0, b->address(), (state)?1:0), [](std::string s){std::cout << s << "\n"; });
					}
				else if (b->command().length()) {
					gui->queueMessage(b->command(), [](std::string s){std::cout << "Response: " << s << "\n"; });
				}
			}
		});

		result = b;
	}
	else if (sc->getName() == "LABEL") {
		EditorLabel *eb = new EditorLabel(window, NamedObject::nextName(0), nullptr, "untitled");
		eb->setDefinition(s);
		eb->setName(eb->getName());
		if (s) s->setName(eb->getName());
		result = eb;
	}
	else if (sc->getName() == "TEXT") {
		EditorGUI *gui = EDITOR->gui();
		EditorTextBox *eb = new EditorTextBox(window, NamedObject::nextName(0), nullptr);
		eb->setDefinition(s);
		s->setName(eb->getName());
		eb->setEditable(true);
		eb->setCallback( [eb, gui](const std::string &value)->bool{
			if (!eb->getRemote()) return true;
			char *rest = 0;
			{
				long val = strtol(value.c_str(),&rest,10);
				if (*rest == 0) {
					gui->queueMessage( gui->getIODSyncCommand(4, eb->getRemote()->address(), (int)val), [](std::string s){std::cout << s << "\n"; });
					return true;
				}
			}
			{
				double val = strtod(value.c_str(),&rest);
				if (*rest == 0)  {
					gui->queueMessage( gui->getIODSyncCommand(4, eb->getRemote()->address(), (float)val), [](std::string s){std::cout << s << "\n"; });
					return true;
				}
			}
			return false;
		});
		result = eb;
	}
	else if (sc->getName() == "IMAGE") {
		GLuint img = gui->getImageId("images/blank");
		EditorImageView *iv = new EditorImageView(window, NamedObject::nextName(0), nullptr, img);
		iv->setDefinition(s);
		s->setName(iv->getName());
		iv->setGridThreshold(20);
		iv->setPixelInfoThreshold(20);
		iv->setImageName("images/blank");
		result = iv;
	}
	else if (sc->getName() == "PLOT") {
		EditorLinePlot *lp = new EditorLinePlot(window, NamedObject::nextName(0), nullptr);
		lp->setDefinition(s);
		s->setName(lp->getName());
		lp->setBufferSize(gui->sampleBufferSize());
		result = lp;
	}
	else if (sc->getName() == "PROGRESS") {
		EditorProgressBar *ep = new EditorProgressBar(window, NamedObject::nextName(0), nullptr);
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

ObjectFactoryButton::ObjectFactoryButton(EditorGUI *screen,
						const std::string type_str, Palette *pal,
						nanogui::Widget *parent, LinkableProperty *lp
						)
	: SelectableButton(type_str, pal, parent, lp->tagName()), gui(screen), properties(lp) {}

nanogui::Widget *ObjectFactoryButton::create(nanogui::Widget *container) const {
	using namespace nanogui;
	Widget *result = 0;
	int object_width = 60;
	int object_height = 25;

	int kind = properties->getKind();
	const std::string &address_str(properties->addressStr());
	const std::string &tag_name(properties->tagName());

	switch (properties->getKind()) {

		case '0': {
			size_t p = tag_name.find(".cmd_");

			if (p != std::string::npos) {
				StructureClass *sc = findClass("BUTTON");
				assert(sc);
				Structure *s = sc->instantiate(gui->getUserWindow()->structure());
				assert(s);

				char *rest = 0;
				int address_int = (int)strtol(address_str.c_str()+2,&rest, 10);

				if (*rest) {
					std::cerr << "Unexpected data in address\n";
					address_int = 0;
				}

				EditorButton *b = new EditorButton(container, NamedObject::nextName(0),
					properties, tag_name.substr(p+5), false);
				b->setSize(Vector2i(object_width, object_height));
				b->setDefinition(s);
				//if (lp) lp->link(new LinkableIndicator(b));
				b->setChangeCallback([b, this] (bool state) {
					if (b->getDefinition()->getKind() == "BUTTON") {
						if (state) {
							gui->queueMessage(
											gui->getIODSyncCommand(0, b->address(), 1), [](std::string s){std::cout << s << "\n"; });
							gui->queueMessage(
											gui->getIODSyncCommand(0, b->address(), 0), [](std::string s){std::cout << s << "\n"; });
						}
					}

				});
				result = b;
			}
			else {
				StructureClass *sc = findClass("INDICATOR");
				assert(sc);
				Structure *s = sc->instantiate(gui->getUserWindow()->structure());
				assert(s);
				EditorButton *b = new EditorButton(container, NamedObject::nextName(0), properties);
				b->setSize(Vector2i(object_width, object_height));
				b->setFlags(Button::ToggleButton);
				LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(tag_name);
				if (lp) lp->link(new LinkableIndicator(b));
				b->setChangeCallback([this,b](bool state) {
					if (b->getDefinition()->getKind() == "BUTTON") {
						gui->queueMessage(gui->getIODSyncCommand(0, b->address(), (state)?1:0), [](std::string s){std::cout << s << "\n"; });
						if (state)
							b->setBackgroundColor(Color(255, 80, 0, 255));
						else
							b->setBackgroundColor(b->theme()->mButtonGradientBotUnfocused   );
					}
				});
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
			EditorButton *b = new EditorButton(container, NamedObject::nextName(0), properties, tag_name, true);
			b->setDefinition(s);
			b->setSize(Vector2i(object_width, object_height));
			b->setFixedSize(Vector2i(object_width, object_height));
			b->setFlags(0);
			LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(tag_name);
			if (lp) lp->link(new LinkableIndicator(b));
			b->setChangeCallback([this,b](bool state) {
				if (b->getDefinition()->getKind() == "BUTTON") {
					gui->queueMessage(gui->getIODSyncCommand(0, b->address(), (state)?1:0), [](std::string s){std::cout << s << "\n"; });
					if (state)
						b->setBackgroundColor(Color(255, 80, 0, 255));
					else
						b->setBackgroundColor(b->theme()->mButtonGradientBotUnfocused   );
				}
			});
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

			EditorLabel *lbl = new EditorLabel(container, NamedObject::nextName(0), properties, "");
			lbl->setName(tag_name);
			lbl->setCaption("");
			lbl->setEnabled(true);
			lbl->setTooltip(tag_name);
			lbl->setSize(Vector2i(object_width, object_height));
			LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(tag_name);
			if (lp) lp->link(new LinkableText(lbl));
			EditorGUI *gui = EDITOR->gui();
			result = lbl;
		}

			break;

		case '4': {
			//new Label(container, tag_name, "sans-bold");
			auto textBox = new EditorTextBox(container, NamedObject::nextName(0), properties);
			textBox->setEditable(true);
			textBox->setSize(Vector2i(object_width, object_height));
			textBox->setFixedSize(Vector2i(object_width, object_height));
			textBox->setTooltip(tag_name);
			//textBox->setDefaultValue("0");
			//textBox->setFontSize(16);
			//textBox->setFormat("[1-9][0-9]*");
			//textBox->setSpinnable(true);
			//textBox->setMinValue(1);
			//textBox->setValueIncrement(1);
			LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(tag_name);
			if (lp) lp->link(new LinkableText(textBox));
			EditorGUI *gui = EDITOR->gui();
			textBox->setCallback( [textBox, gui, lp](const std::string &value)->bool{
				char *rest = 0;
				{
					long val = strtol(value.c_str(),&rest,10);
					if (*rest == 0) {
						gui->queueMessage( gui->getIODSyncCommand(4, textBox->getRemote()->address(), (int)val), [](std::string s){std::cout << s << "\n"; });
						return true;
					}
				}
				{
					double val = strtod(value.c_str(),&rest);
					if (*rest == 0)  {
						gui->queueMessage( gui->getIODSyncCommand(4, textBox->getRemote()->address(), (float)val), [](std::string s){std::cout << s << "\n"; });
						return true;
					}
				}
				if (lp->dataType() == CircularBuffer::STR)
					gui->queueMessage( gui->getIODSyncCommand(4, textBox->getRemote()->address(), value.c_str()), [](std::string s){std::cout << s << "\n"; });
				return false;
			});
			result = textBox;
		}

			break;

		default:
			;
	}
	return result;
}
