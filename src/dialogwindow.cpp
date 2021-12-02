#include <string>

#include <nanogui/theme.h>
#include <nanogui/widget.h>

#include "dialogwindow.h"
#include "editorgui.h"
#include "helper.h"
#include "editorwidget.h"
#include "namedobject.h"
#include "widgetfactory.h"

DialogWindow::DialogWindow(EditorGUI *screen, nanogui::Theme *theme) : nanogui::Window(screen), gui(screen) {
	using namespace nanogui;
	setTheme(theme);
	setFixedSize(Vector2i(gui->width()/2, gui->height()/2));
	setSize(Vector2i(180,240));
	setPosition(Vector2i(gui->width()/4, gui->height()/4));
	setTitle("Dialog");
	setVisible(true);
}

void DialogWindow::clear() {
	StructureClass *sc = current_structure ? findClass(current_structure->getKind()) : nullptr;
	int n = childCount();
	int idx = 0;
	while (n--) {
		NamedObject *no = dynamic_cast<NamedObject *>(childAt(idx));
		EditorWidget *ew = dynamic_cast<EditorWidget*>(no);
		if (ew && ew->getRemote()) {
			ew->getRemote()->unlink(ew);
		}
		if (ew && sc) { LinkableObject::unlink(sc->getName(), ew); }
		removeChild(idx);
	}
}

void DialogWindow::setStructure( Structure *s) {
	if (!s) return;

	StructureClass *sc = findClass(s->getKind());
	if (!sc) {
		std::cerr << "no structure class '" << s->getKind() << "' found\n";
		return;
	}
	if ( s->getKind() != "SCREEN" && (!sc || sc->getBase() != "SCREEN") ) return;
	clear();
	loadStructure(s);
	current_structure = s;
	gui->performLayout();
}

void DialogWindow::loadStructure(Structure *s) {
	StructureClass *sc = findClass(s->getKind());
	if (sc && (s->getKind() == "SCREEN" || sc->getBase() == "SCREEN") ) {
		if (sc && !s->getStructureDefinition())
			s->setStructureDefinition(sc);
		int pnum = 0;
		nanogui::Vector2i offset;
		nanogui::Vector2i frame_size;
		for (auto param : sc->getLocals()) {
			++pnum;
			Structure *element = param.machine;
			if (!element) {
				continue;
			}
			std::string kind = element->getKind();
			if (kind == "FRAME") {
				StructureClass *element_class = param.machine->getStructureDefinition();
				if (!element_class) {
					std::string kind = element->getKind();
					StructureClass *element_class = findClass(kind);
					param.machine->setStructureDefinition(element_class);
				}
				auto properties = element->getProperties();
				Value vx = element->getValue("pos_x");
				Value vy = element->getValue("pos_y");
				Value w = element->getValue("width");
				Value h = element->getValue("height");
				long x, y;
				if (vx.asInteger(x) && vy.asInteger(y)) {
					offset = nanogui::Vector2i(x,y);
				}
				if (w.asInteger(x) && h.asInteger(y)) {
					frame_size = nanogui::Vector2i(x, y);
					setFixedSize(frame_size);
					setSize(frame_size);
					auto pos = nanogui::Vector2i((gui->width() - frame_size.x())/2, (gui->height() - frame_size.y())/2);
					setPosition(pos);

				}
				break;
			}
		}
		pnum = 0;
		for (auto param : sc->getLocals()) {
			++pnum;
			Structure *element = param.machine;
			if (!element) {
				std::cout << "Warning: no structure for parameter " << pnum << "of " << s->getName() << "\n";
				continue;
			}
			StructureClass *element_class = param.machine->getStructureDefinition();
			if (!element_class) {
				std::string kind = element->getKind();
				element_class = findClass(kind);
				element->setStructureDefinition(element_class);
				if (!element_class) {
					std::cout << "cannot find structure definition for " << kind << " when loading screen " << s->getName() << "\n";
				}
			}

			WidgetParams params(s, this, element, gui, offset);

			if (element_class &&element_class->isExtension("LABEL")) {
				createLabel(params);
			}
			if (element_class &&element_class->isExtension("IMAGE")) {
				createImage(params);
			}
			if (element_class &&element_class->isExtension("PROGRESS")) {
				createProgress(params);
			}
			if (element_class &&element_class->isExtension("TEXT")) {
				createText(params);
			}
			else if (element_class && element_class->isExtension("PLOT")) {
				createPlot(params);
			}
			else if (element_class && element_class->isExtension("COMBOBOX")) {
				createComboBox(params);
			}
			else if (element_class && (element_class->isExtension("BUTTON") || element_class->isExtension("INDICATOR"))) {
				createButton(params);
			}
		}
	}
}

