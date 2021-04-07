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
	setFixedSize(Vector2i(screen->width()/2, screen->height()/2));
	setSize(Vector2i(180,240));
	setPosition(Vector2i(screen->width()/4, screen->height()/4));
	setTitle("Dialog");
	setVisible(true);
}

void DialogWindow::clear() {
	int n = childCount();
	int idx = 0;
	while (n--) {
		NamedObject *no = dynamic_cast<NamedObject *>(childAt(idx));
		EditorWidget *ew = dynamic_cast<EditorWidget*>(no);
		if (ew && ew->getRemote()) {
			ew->getRemote()->unlink(ew);
		}
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
		for (auto param : sc->getLocals()) {
			++pnum;
			Structure *element = param.machine;
			if (!element) {
				std::cout << "Warning: no structure for parameter " << pnum << "of " << s->getName() << "\n";
				continue;
			}
			std::string kind = element->getKind();
			StructureClass *element_class = findClass(kind);

			WidgetParams params(s, this, element, gui);

			if (kind == "LABEL") {
				createLabel(params);
			}
			if (kind == "IMAGE") {
				createImage(params);
			}
			if (kind == "PROGRESS") {
				createProgress(params);
			}
			if (kind == "TEXT") {
				createText(params);
			}
			else if (kind == "PLOT" || (element_class &&element_class->getBase() == "PLOT") ) {
				createPlot(params);
			}
			else if (kind == "BUTTON" || kind == "INDICATOR") {
				createButton(params);
			}
		}
	}
}

