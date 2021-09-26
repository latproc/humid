#include "userwindowwin.h"
#include "userwindow.h"
#include "editorgui.h"
#include "editorobject.h"
#include "skeleton.h"
#include "editor.h"
#include "editorwidget.h"
#include "structureswindow.h"
#include "objectwindow.h"
#include "editorlineplot.h"
#include "helper.h"
#include "editorlabel.h"
#include "editorimageview.h"
#include "editorbutton.h"
#include "editortextbox.h"
#include "editorprogressbar.h"
#include "propertyformhelper.h"
#include "screenswindow.h"
#include "widgetfactory.h"

using namespace nanogui;

class LinkableProperty;
std::map<std::string, LinkableProperty *> remotes;

extern void loadProjectFiles(std::list<std::string> &files_and_directories);

static bool stringEndsWith(const std::string &src, const std::string ending) {
	size_t l_src = src.length();
	size_t l_end = ending.length();
	if (l_src < l_end) return false;
	return src.substr(src.end() - l_end - src.begin()) == ending;
}

void fixElementPosition(nanogui::Widget *w, const SymbolTable &properties) {
	Value vx = properties.find("pos_x");
	Value vy = properties.find("pos_y");
	if (vx != SymbolTable::Null && vx != SymbolTable::Null) {
		long x, y;
		if (vx.asInteger(x) && vy.asInteger(y)) w->setPosition(nanogui::Vector2i(x,y));
	}
}

void fixElementSize(nanogui::Widget *w, const SymbolTable &properties) {
	Value vx = properties.find("width");
	Value vy = properties.find("height");
	if (vx != SymbolTable::Null && vx != SymbolTable::Null) {
		long x, y;
		if (vx.asInteger(x) && vy.asInteger(y)) {
			w->setSize(nanogui::Vector2i(x,y));
			w->setFixedSize(nanogui::Vector2i(x,y));
		}
	}
}

void UserWindowWin::draw(NVGcontext *ctx) {
	nvgSave(ctx);
//    glMatrixMode(GL_PROJECTION);
//    glLoadIdentity();
//	float aspect = (float)width() / (float)height();
//    glOrtho(3.0 * aspect, 3.0 * aspect, -3.0, 3.0, 1.0, 50.0);
//	glMatrixMode(GL_MODELVIEW);
//    glLoadIdentity();
	float scale = 1.0f;
	nvgScale(ctx, scale, scale);
	float x_offset = this->width() * (1.0f-scale) / 2.0;
	float y_offset = this->height() * (1.0f-scale) / 2.0;
	nvgTranslate(ctx, x_offset, y_offset);
	nanogui::Window::draw(ctx);
	//nvgScale(ctx, 1.0f/0.9f, 1.0f/1.9f);
	nvgRestore(ctx);

}

bool UserWindowWin::keyboardEvent(int key, int scancode , int action, int modifiers) {
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		if (EDITOR && EDITOR->isEditMode()) {
			UserWindow *uw = gui->getUserWindow();
			if (action == GLFW_PRESS && (key == GLFW_KEY_BACKSPACE || key == GLFW_KEY_DELETE) ) {
				if (uw && uw->hasSelections()) {
					uw->deleteSelections();
					return false;
				}
			}
			else if ( action == GLFW_PRESS && key == GLFW_KEY_TAB && childCount() > 1) {
				bool backward = modifiers & GLFW_MOD_SHIFT;
				int start = current_item;
				int n = childCount();
				int i = start;
				int maxpos = -1;
				int minpos = -1;
				int maxidx = start;
				int minidx = start;
				int start_pos = -1;
				if (start >= 0) {
					nanogui::Widget *w = childAt(start);
					EditorWidget *ew = dynamic_cast<EditorWidget*>(childAt(start));
					if (ew) { start_pos = ew->tabPosition(); }
				}


				// search for a widget with a lower tab position
				while (--i != start) {
					if (i < 0) i = n-1;
					if (i == start) break;
					nanogui::Widget *w = childAt(i);
					EditorWidget *ew = dynamic_cast<EditorWidget*>(w);
					if (!ew) continue;
					int pos = ew->tabPosition();
					if (pos>0) {
						if (backward) {
							if (pos > minpos && (start_pos == -1 || pos <= start_pos) ) { minpos = pos; minidx = i; }
							if ( (pos > maxpos || maxpos == -1) && pos >= start_pos ) { maxpos = pos; maxidx = i; }
						}
						else {
							if ( (pos < minpos || minpos == -1) && (start_pos == -1 || pos <= start_pos) ) { minpos = pos; minidx = i; }
							if ( (pos < maxpos || maxpos == -1) && pos >= start_pos ) { maxpos = pos; maxidx = i; }
						}
					}
				}
				int new_sel = current_item;
				if (backward) {
					if (minidx != start) new_sel = minidx; else if (maxidx != start) new_sel = maxidx;
				}
				else {
					if (maxidx != start) new_sel = maxidx; else if (minidx != start) new_sel = minidx;
				}
				if (new_sel >= 0 && new_sel != start) {
					EditorWidget *ew = 0;
					if (start >= 0) ew = dynamic_cast<EditorWidget*>(childAt(start));
					if (ew) ew->deselect();
					ew = dynamic_cast<EditorWidget*>(childAt(new_sel));
					if (ew) ew->select();
				}
			}
			else if ( uw->hasSelections() &&
						( key == GLFW_KEY_LEFT || key == GLFW_KEY_UP || key == GLFW_KEY_RIGHT || key == GLFW_KEY_DOWN ) ) {
				if (EDITOR->getDragHandle()) EDITOR->getDragHandle()->setVisible(false);
				for (auto item : uw->getSelected() ) {
					nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(item);
					if (!w) continue;
					switch (key) {
						case GLFW_KEY_LEFT:
							if (w->position().x()> 0)
								w->setPosition( nanogui::Vector2i(w->position().x()-1, w->position().y()) );
							break;
						case GLFW_KEY_RIGHT:
							if (w->position().x() + w->width() < w->parent()->width())
								w->setPosition( nanogui::Vector2i(w->position().x()+1, w->position().y()) );
							break;
						case GLFW_KEY_UP:
							if (w->position().y()> w->parent()->theme()->mWindowHeaderHeight)
								w->setPosition( nanogui::Vector2i(w->position().x(), w->position().y()-1) );
							break;
						case GLFW_KEY_DOWN:
							if (w->position().y() + w->height() < w->parent()->height())
								w->setPosition( nanogui::Vector2i(w->position().x(), w->position().y()+1) );
							break;
						default:
							break;
					}
				}
			}
		}
		bool handled = false;
		for (auto item : children() ) {
			nanogui::LinePlot *lp = dynamic_cast<nanogui::LinePlot*>(item);
			if (lp && lp->focused()) { lp->handleKey(key, scancode, action, modifiers); handled = true; }
		}
		if (!handled) return Window::keyboardEvent(key, scancode, action, modifiers);
	}
	return true;
}

bool UserWindowWin::mouseEnterEvent(const Vector2i &p, bool enter) {
	bool res = Window::mouseEnterEvent(p, enter);
	StructuresWindow *sw = EDITOR->gui()->getStructuresWindow();;
	ObjectWindow *ow = EDITOR->gui()->getObjectWindow();
	if (!focused() && enter && EDITOR->isEditMode() && ( (sw && sw->hasSelections()) || (ow && ow->hasSelections() )))
		requestFocus();

	return res;
}

UserWindow::UserWindow(EditorGUI *screen, nanogui::Theme *theme, UserWindowWin *uww)
: Skeleton(screen, uww), LinkableObject(), gui(screen), current_layer(0), mDefaultSize(1024,768), current_structure(0) {
	using namespace nanogui;
	window->setTheme(theme);
	GLFWmonitor* primary = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(primary);
	long width = screen->size().x();
	long height = screen->size().y();
	{
		const Value width_v = EditorGUI::systemSettings()->getProperties().find("panel_width");
		const Value height_v = EditorGUI::systemSettings()->getProperties().find("panel_height");
		width_v.asInteger(width);
		height_v.asInteger(height);
	}

	mDefaultSize.x() = width;
	mDefaultSize.y() = height;
	window->setFixedSize(mDefaultSize);
	window->setSize(mDefaultSize);
	window->setVisible(false);
	window->setTitle("Panel Window");
	long window_x = gui->size().x() > width ? (gui->size().x() - width)/2 : 0;
	long window_y = gui->size().y() > height ? (gui->size().y() - height)/2 : 0;
	{
		const Value x_v = EditorGUI::systemSettings()->getProperties().find("panel_left");
		const Value y_v = EditorGUI::systemSettings()->getProperties().find("panel_top");
		x_v.asInteger(window_x);
		y_v.asInteger(window_y);
	}

	window->setPosition(nanogui::Vector2i(window_x, window_y));
	push(window);
}

void UserWindow::refresh() { window->performLayout(gui->nvgContext()); }

void UserWindow::update(const Value &value) {
	if (!EDITOR->isEditMode()) {
		Structure *s = findScreen(value.asString());
		if (s)
			EditorGUI::systemSettings()->getProperties().add("active_screen", Value(s->getName(), Value::t_string));
	}
}

void UserWindow::startEditMode() {
	for (auto child : window->children()) {
		EditorWidget *ew = dynamic_cast<EditorWidget*>(child);
		if (ew && ew->asWidget()) ew->asWidget()->setVisible(true);
	}
}

void UserWindow::endEditMode() { 
	clearSelections();
}

void UserWindow::select(Selectable * w) {
	if (!dynamic_cast<nanogui::Widget*>(w)) return;
	Palette::select(w);
	UserWindowWin *wnd = dynamic_cast<UserWindowWin*>(window);
	nanogui::Widget *widget = dynamic_cast<nanogui::Widget *>(w);
	if (widget && wnd) wnd->setCurrentItem(window->childIndex(widget));
}
void UserWindow::deselect(Selectable *w) {
	if (!dynamic_cast<nanogui::Widget*>(w)) return;
	Palette::deselect(w);
	UserWindowWin *wnd = dynamic_cast<UserWindowWin*>(window);
	if (wnd) wnd->setCurrentItem(-1);
}

void UserWindow::fixLinks(LinkableProperty *lp) {
	int n = window->childCount();
	int idx = 0;
	while (n--) {
		EditorWidget *ew = dynamic_cast<EditorWidget *>(window->childAt(idx));
		if (ew->getDefinition()) {
			const Value &r = ew->getDefinition()->getProperties().find("Remote");
			if (r != SymbolTable::Null && r.asString() == lp->tagName()) {
				if (ew->getRemote()) ew->getRemote()->unlink(ew->getRemote());
				ew->setRemote(lp);
				ew->setProperty("Remote", lp->tagName());
			}
		}
	}
}

void UserWindow::setStructure( Structure *s) {
	if (!s) return;

	StructureClass *sc = findClass(s->getKind());
	if ( s->getKind() != "SCREEN" && (!sc || sc->getBase() != "SCREEN") ) return;

	// save current settings
	if (structure()) {
		std::list<std::string>props;
		std::map<std::string, std::string> property_map;
		loadPropertyToStructureMap(property_map);
		getPropertyNames(props);
		for (auto prop : props) {
			Value v = getPropertyValue(prop);
			if (v != SymbolTable::Null) {
				auto item = property_map.find(prop);
				if (item != property_map.end())
					structure()->getProperties().add((*item).second, v);
				else
					structure()->getProperties().add(prop, v);
			}
		}
	}

	clearSelections();
	clear();

	nanogui::DragHandle *drag_handle = EDITOR->getDragHandle();
	PropertyMonitor *pm  = nullptr;
	if (drag_handle) {
		drag_handle->incRef();
		window->removeChild(drag_handle);
		pm = drag_handle->propertyMonitor();
		drag_handle->setPropertyMonitor(0);
	}

	loadStructure(s);
	current_structure = s;

	window->addChild(drag_handle);
	if (drag_handle) {
	  drag_handle->setPropertyMonitor(pm);
	  drag_handle->decRef();
	}
	gui->performLayout();
}

NVGcontext* UserWindow::getNVGContext() { return gui->nvgContext(); }

void UserWindow::deleteSelections() {
	if (EDITOR->getDragHandle()) EDITOR->getDragHandle()->setVisible(false);
	getWindow()->requestFocus();
	std::set<Selectable *> to_delete;
	for (auto sel : getSelected()) {
		to_delete.insert(sel);
	}
	clearSelections();
	gui->getPropertyWindow()->update();

	StructureClass *screen_sc = structure()->getStructureDefinition();
	for (auto sel : to_delete) {
		nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(sel);
		auto no = dynamic_cast<NamedObject*>(sel);
		if (no) {
			auto iter = screen_sc->getLocals().begin();
			while (iter != screen_sc->getLocals().end()) {
				auto & param = *iter;
				if (param.val.sValue == no->getName()) {
					iter = screen_sc->getLocals().erase(iter);
				}
				else {
					iter++;
				}
			}
		}
		if (w) getWindow()->removeChild(w);
	}
}

void UserWindow::clear() {
	nanogui::DragHandle *drag_handle = EDITOR->getDragHandle();
	if (drag_handle) drag_handle->setVisible(false);
	drag_handle->incRef();
	window->removeChild(drag_handle);
	PropertyMonitor *pm = drag_handle->propertyMonitor();
	drag_handle->setPropertyMonitor(0);

	auto sc = structure() ? structure()->getStructureDefinition() : nullptr;
	int n = window->childCount();
	int idx = 0;
	while (n--) {
		NamedObject *no = dynamic_cast<NamedObject *>(window->childAt(idx));
		EditorWidget *ew = dynamic_cast<EditorWidget*>(no);
		if (ew && ew->getRemote()) {
			ew->getRemote()->unlink(ew);
		}
		if (ew && sc) { LinkableObject::unlink(sc->getName(), ew); }
		window->removeChild(idx);
	}
	window->addChild(drag_handle);

	drag_handle->setPropertyMonitor(pm);
	drag_handle->decRef();

	window->performLayout( gui->nvgContext() );
}

CircularBuffer *UserWindow::getValues(const std::string name) {
	auto iter = data.find(name);
	if (iter != data.end()) {
		const std::pair<std::string, CircularBuffer *> &node = *iter;
		return node.second;
	}
	return 0;
}

CircularBuffer * UserWindow::addDataBuffer(const std::string name, CircularBuffer::DataType dt, size_t len) {
	auto found = data.find(name);
	if (found != data.end()) {
		CircularBuffer *buf = (*found).second;
		if (buf->getDataType() == dt) return buf;
		CircularBuffer *new_buf = new CircularBuffer(len, dt);
		data[name] = new_buf;
		delete buf;
		return new_buf;
	}
	else {
		CircularBuffer *buf = new CircularBuffer(len, dt);
		data[name] = buf;
		return buf;
	}
}

// TBD remove this hack (see lineplot)
CircularBuffer *UserWindow::createBuffer(const std::string name) {
	CircularBuffer *res = getValues(name);
	if (!res) {
		LinkableProperty *lp = gui->findLinkableProperty(name);
		if (lp)
			res = addDataBuffer(name, lp->dataType(), gui->sampleBufferSize());
	}
	return res;
}

void UserWindow::loadStructure(Structure *s) {
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

			WidgetParams params(s, window, element, gui, nanogui::Vector2i(0,0));

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
			else if (kind == "FRAME") {
				createFrame(params);
			}
			else if (kind == "BUTTON" || kind == "INDICATOR") {
				createButton(params);
			}
		}
	}
	if (s->getKind() == "REMOTE") {
		const Value &rname(s->getProperties().find("NAME"));
		if (rname != SymbolTable::Null) {
			LinkableProperty *lp = gui->findLinkableProperty(rname.asString());
			if (lp) {
				remotes[s->getName()] = lp;
			}
		}
	}
	if (s->getKind() == "SCREEN" || (sc && sc->getBase() == "SCREEN") ) {
		const Value &title(s->getProperties().find("caption"));
		//if (title != SymbolTable::Null) window->setTitle(title.asString());
		PanelScreen *ps = getActivePanel();
		if (ps) ps->setName(s->getName());

		ThemeWindow *tw = EDITOR->gui()->getThemeWindow();
		nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(EDITOR->gui()->getUserWindow()->getWindow());
		if (w && tw && tw->getWindow()->visible() ) {
			tw->loadTheme(w->theme());
		}
		PropertyWindow *prop = EDITOR->gui()->getPropertyWindow();
		if (prop && prop->getWindow()->visible()) {
			if (!EDITOR->isEditMode())
				prop->getWindow()->setVisible(false);
			else
				prop->update();
			EDITOR->gui()->needsUpdate();
		}
		s->setChanged(false);
	}
}

void UserWindow::load(const std::string &path) {
	std::list<std::string> files;
	files.push_back(path);
	loadProjectFiles(files);
}

void UserWindow::update() {
	UserWindowWin * w = dynamic_cast<UserWindowWin*>( getWindow() );
	if (w) w->update();
}

void UserWindowWin::update() {
	// if the window contains a line plot, that object should be updated
	for (auto child : children()) {
		nanogui::LinePlot *lp = dynamic_cast<nanogui::LinePlot*>(child);
		if (lp) lp->update();
	}
}

void UserWindow::getPropertyNames(std::list<std::string> &names) {
	names.push_back("Screen Width");
	names.push_back("Screen Height");
	names.push_back("Screen Id");
	names.push_back("File Name");
	names.push_back("Visibility");
	names.push_back("Channel");
}

void UserWindow::loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) {
	property_map["Screen Width"] = "screen_width";
	property_map["Screen Height"] = "screen_height";
	property_map["Screen Id"] = "screen_id";
	property_map["File Name"] = "file_name";
	property_map["Visibility"] = "visibility";
	property_map["Channel"] = "channel";
}


void UserWindow::loadProperties(PropertyFormHelper *properties) {
	UserWindow *uw = this;
	{
		std::string label("Screen Title");
		properties->addVariable<std::string>(label,
				 [uw](std::string value) {
					 PanelScreen *ps = uw->getActivePanel();
					 if (ps) {
						 uw->getWindow()->setTitle(value);
						 assert(uw->structure());
						 uw->structure()->getProperties().add("Title", value);
					 }
					 uw->getWindow()->requestFocus();
				 },
				 [uw]()->std::string{
					 PanelScreen *ps = uw->getActivePanel();
					 if (ps) return uw->getWindow()->title();
					 return "";
				 });
	}
	{
	std::string label("Screen Name");
	properties->addVariable<std::string>(label,
							 [uw](std::string value) {
								 PanelScreen *ps = uw->getActivePanel();
								 if (ps) {
									 ps->setName(value);
									 assert(uw->structure());
									 uw->structure()->setName(value);
								 }
								 uw->getWindow()->requestFocus();
								 if (uw->app()->getScreensWindow()) {
									 uw->app()->getScreensWindow()->updateSelectedName();
								 }
							 },
							 [uw]()->std::string{
								 PanelScreen *ps = uw->getActivePanel();
								 if (ps) return ps->getName();
								 return "";
							 });
	}
	{
	std::string label("File Name");
	properties->addVariable<std::string>(label,
							 [uw](std::string value) {
								 std::cout << "setting file name for " << uw->structure()->getName() << " to " << value << "\n";
								 	if (uw->structure())
									uw->structure()->getInternalProperties().add("file_name", Value(value, Value::t_string));
							 },
							 [uw]()->std::string{
									if (!uw->structure()) return "";
									const Value &vx = uw->structure()->getInternalProperties().find("file_name");
									if (vx != SymbolTable::Null) return vx.asString();
									return "";
							 });
	}
	{
	std::string label("Screen Class");
	properties->addVariable<std::string>(label,
							 [uw](std::string value) {
								 assert(uw->structure());
								 StructureClass *sc = findClass(value);
								 if (sc && sc != uw->structure()->getStructureDefinition())  {
									 std::cout << "Error: structure class " << value << " already exists\n";
									 return;
								 }
								 if (uw->structure()->getStructureDefinition())
								 uw->structure()->getStructureDefinition()->setName(value);
							 },
							 [uw]()->std::string{
								 ScreensWindow *sw = (uw->app()->getScreensWindow());
								 if (!sw) return "";
								 if (!uw->structure()) return "Untitled";
								  if (!uw->structure()->getStructureDefinition()) return "Unknown";
								 return uw->structure()->getStructureDefinition()->getName();
							 });
	}
	{
	std::string label("Channel");
	properties->addVariable<std::string>(label,
		[uw](std::string value) {
			if (uw) {
				Structure *s = uw->structure();
				if (s) s->getProperties().add("channel", value);
			}
		},
		[uw]()->std::string{
			if (uw) {
				Structure *s = uw->structure();
				return s ? s->getProperties().lookup("channel").asString() : "";
			}
			return "";
		});
	}
	{
	std::string label("Class File");
	properties->addVariable<std::string>(label,
							 [uw](std::string value) {
								 	if (uw->structure() && uw->structure()->getStructureDefinition())
										uw->structure()->getStructureDefinition()->getInternalProperties()
											.add("file_name", Value(value, Value::t_string));
							 },
							 [uw]()->std::string{
									if (!uw->structure() || uw->structure()->getStructureDefinition()) return "";
									const Value &vx = uw->structure()->getStructureDefinition()->getInternalProperties().find("file_name");
									if (vx != SymbolTable::Null)
										return vx.asString();
									return "";
							 });
	}
	{
	std::string label = "Window Width";
	properties->addVariable<int>(label,
							 [uw](int value) mutable {
								 nanogui::Window *w = uw->getWindow();
								 w->setWidth(value);
								 w->setFixedWidth(value);
								 },
							 [uw]()->int{ return uw->getWindow()->width(); });
	label = "Window Height";
	properties->addVariable<int>(label,
							 [uw](int value) mutable {
								 nanogui::Window *w = uw->getWindow();
								 w->setHeight(value);
								 w->setFixedHeight(value);
								 },
							 [uw]()->int{ return uw->getWindow()->height(); });
	label = "Screen Width";
	properties->addVariable<int>(label,
							 [uw](int value) mutable {
								 nanogui::Window *w = uw->getWindow();
								 w->setWidth(value);
								 w->setFixedWidth(value);
								 },
							 [uw]()->int{ return uw->getWindow()->width(); });
	label = "Screen Height";
	properties->addVariable<int>(label,
							 [uw](int value) mutable {
								 nanogui::Window *w = uw->getWindow();
								 w->setHeight(value);
								 w->setFixedHeight(value);
								 },
							 [uw]()->int{ return uw->getWindow()->height(); });
  properties->addVariable<std::string> (
    "Visibility",
		[&,this,properties](std::string value) {
			LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(value);
			// TODO support linking to a screen's visibility
			// if (visibility) visibility->unlink(this);
			// visibility = lp;
			// if (lp) { lp->link(new LinkableVisibility(this)); }
			},
		[&]()->std::string{
			// TODO return the visibility of the loaded screen
			//return visibility ? visibility->tagName() : ""; 
		return "";
	});
	label = "Screen Id";
	properties->addVariable<int>(label,
		[uw](int value) mutable {
			nanogui::Window *w = uw->getWindow();
			Structure *s = uw->structure();
			if (s) s->getProperties().add("screen_id", value);
		},
		[uw]()->int{
			Structure *s = uw->structure();
			if (s) {
				const Value &v(s->getProperties().find("screen_id"));
				long res = 0;
				if (v.asInteger(res)) return res;
			}
			return 0;
		});
	}
}

Value UserWindow::getPropertyValue(const std::string &prop) {
	if (prop == "Screen Width") return getWindow()->width();
	if (prop == "Screen Height") return getWindow()->height();
	if (prop == "Window Width") return getWindow()->width();
	if (prop == "Window Height") return getWindow()->height();
	Structure *current_screen = structure();
	if (prop == "File Name" && current_screen)
		return current_screen->getInternalProperties().find("file_name");
	if (prop == "Channel") return current_screen->getProperties().lookup("channel");
	return SymbolTable::Null;
}

