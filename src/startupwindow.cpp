#include <nanogui/theme.h>
#include <nanogui/button.h>
#include <nanogui/layout.h>
#include <nanogui/label.h>

#include "skeleton.h"
#include "editorgui.h"
#include "startupwindow.h"
#include "listpanel.h"

StartupWindow::StartupWindow(EditorGUI *screen, nanogui::Theme *theme) : Skeleton(screen), gui(screen) {
	using namespace nanogui;
	window->setTheme(theme);
	Label *itemText = new Label(window, "", "sans-bold");
	itemText->setPosition(Vector2i(40, 40));
	itemText->setSize(Vector2i(260, 20));
	itemText->setFixedSize(Vector2i(260, 20));
	itemText->setCaption("Select a project to open");
	itemText->setVisible(true);

	project_box = new ListPanel(window);
	project_box->setSize(Vector2i(300, 100));
	project_box->setPosition(Vector2i(50,100));
	project_box->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Fill));

	Button *newProjectButton = new Button(project_box, "New");
	newProjectButton->setPosition(Vector2i(80, 80));
	newProjectButton->setFixedSize(Vector2i(120, 40));
	newProjectButton->setSize(Vector2i(120, 40));
	newProjectButton->setCallback([this, newProjectButton] {
		newProjectButton->parent()->setVisible(false);
		this->gui->setState(EditorGUI::GUICREATEPROJECT);
	});
	newProjectButton->setEnabled(true);

	window->setFixedSize(Vector2i(400, 400));
	window->setSize(Vector2i(400, 400));
	window->setVisible(false);
}
