//
//  ObjectWindow.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
/* This file is formatted with:
    astyle --style=kr --indent=tab=2 --one-line=keep-blocks --brackets=break-closing
*/
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/label.h>
#include <nanogui/checkbox.h>
#include <nanogui/button.h>
#include <nanogui/toolbutton.h>
#include <nanogui/popupbutton.h>
#include <nanogui/combobox.h>
#include <nanogui/progressbar.h>
#include <nanogui/entypo.h>
#include <nanogui/messagedialog.h>
#include <nanogui/textbox.h>
#include <nanogui/slider.h>
#include <nanogui/imagepanel.h>
#include <nanogui/imageview.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/colorwheel.h>
#include <nanogui/graph.h>
#include <nanogui/formhelper.h>
#include <nanogui/theme.h>
#include <nanogui/tabwidget.h>
#include <nanogui/common.h>
#if defined(_WIN32)
#include <windows.h>
#endif
#include <nanogui/opengl.h>
#include <nanogui/glutil.h>

#include <iostream>
#include <fstream>
#include <locale>
#include <string>
#include <vector>
#include <set>
#include <functional>
#include <libgen.h>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <lib_clockwork_client.hpp>
#include "includes.hpp"
#include "list_panel.h"
#include "userwindowwin.h"
#include "toolbar.h"
#include "startupwindow.h"
#include "viewswindow.h"

using std::cout;
using std::cerr;
using std::endl;
using std::locale;
using nanogui::Vector2i;
using nanogui::Vector2f;
using Eigen::Vector2d;
using Eigen::MatrixXd;
using Eigen::Matrix3d;

void loadProjectFiles(std::list<std::string> &files_and_directories);

void UserWindowWin::draw(NVGcontext *ctx) {
	nvgSave(ctx);
	// TODO: add a scale  and offset setting for the user window and use them here for zooming and panning
	// Note: NanoGUI widgets seem to have a scroll event so zooming/panning should hook into that
	// TODO: (possibly?) add mouseMotionEvent, mouseButtonEvent etc to rescale mouse clicks etc
	//	float scale = 0.5f;
	//	int offset = width() * (1.0f - scale) / 2.0f;
	//  nvgScale(ctx, 0.5f, 0.5f);
	//	nvgTranslate(ctx, offset, offset);
	nanogui::Window::draw(ctx);
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
	//else if (focused() && !enter)
	//		setFocused(false);

	return res;
}


UserWindow::UserWindow(EditorGUI *screen, nanogui::Theme *theme, UserWindowWin *uww)
: Skeleton(screen, uww), LinkableObject(0), gui(screen), current_layer(0), mDefaultSize(1024,768), current_structure(0) {
	using namespace nanogui;
	gui = screen;
	window->setTheme(theme);
	GLFWmonitor* primary = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(primary);
	mDefaultSize.x() = mode->width;
	mDefaultSize.y() = mode->height;
	window->setFixedSize(mDefaultSize);
	window->setSize(mDefaultSize);
	window->setVisible(false);
	window->setTitle("");
	window->setPosition(nanogui::Vector2i(0,0));
	push(window);
}

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
	nanogui::DragHandle *drag_handle = EDITOR->getDragHandle();
	PropertyMonitor *pm  = nullptr;
	if (drag_handle) {
		drag_handle->incRef();
		window->removeChild(drag_handle);
		pm = drag_handle->propertyMonitor();
		drag_handle->setPropertyMonitor(0);
	}
	int n = window->childCount();
	while (n--) {
		window->removeChild(0);
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
	auto to_delete = getSelected();
	clearSelections();
	for (auto sel : to_delete) {
		nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(sel);
		EditorWidget *ew = dynamic_cast<EditorWidget*>(w);
		if (ew) {
			Structure *s = ew->getDefinition();
			current_structure->getStructureDefinition()->removeLocal(s);
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

	int n = window->childCount();
	int idx = 0;
	while (n--) {
		NamedObject *no = dynamic_cast<NamedObject *>(window->childAt(idx));
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

CircularBuffer * UserWindow::addDataBuffer(const std::string name, Humid::DataType dt, size_t len) {
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

void UserWindow::loadStructure( Structure *s) {
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
			const Value &remote(element->getProperties().find("remote"));
			const Value &vis(element->getProperties().find("visibility"));
			const Value &connection(element->getProperties().find("connection"));
			const Value &border(element->getProperties().find("border"));
			const Value &font_size_val(element->getProperties().find("font_size"));
			LinkableProperty *lp = nullptr;
			if (remote != SymbolTable::Null)
					lp = gui->findLinkableProperty(remote.asString());
			LinkableProperty *visibility = nullptr;
			if (vis != SymbolTable::Null)
					visibility = gui->findLinkableProperty(vis.asString());

			bool wrap = false;
			{
				const Value &wrap_v(element->getProperties().find("wrap"));
				if (wrap_v != SymbolTable::Null) wrap_v.asBoolean(wrap);
			}

			bool ivis = false;
			{
				const Value &ivis_v(element->getProperties().find("inverted_visibility"));
				if (ivis_v != SymbolTable::Null) ivis_v.asBoolean(ivis);
			}
			long font_size = 0;
			if (font_size_val != SymbolTable::Null) font_size_val.asInteger(font_size);
			const Value &format_val(element->getProperties().find("format"));
			const Value &value_type_val(element->getProperties().find("value_type"));
			long value_type = -1;
			if (value_type_val != SymbolTable::Null) value_type_val.asInteger(value_type);
			const Value &scale_val(element->getProperties().find("value_scale"));
			double value_scale = 1.0f;
			if (scale_val != SymbolTable::Null) scale_val.asFloat(value_scale);
			long tab_pos = 0;
			const Value &tab_pos_val(element->getProperties().find("tab_pos"));
			if (tab_pos_val != SymbolTable::Null) tab_pos_val.asInteger(tab_pos);
			if (kind == "LABEL") {
                const Value &caption_v( (lp) ? lp->value() : (remote != SymbolTable::Null) ? "" : element->getProperties().find("caption"));
				EditorLabel *el = new EditorLabel(s, window, element->getName(), lp,
												  (caption_v != SymbolTable::Null)?caption_v.asString(): "");
				el->setName(element->getName());
				el->setDefinition(element);
				fixElementPosition( el, element->getProperties());
				fixElementSize( el, element->getProperties());
				if (connection != SymbolTable::Null) {
                    el->setRemoteName(remote.asString());
					el->setConnection(connection.asString());
				}
				if (font_size) el->setFontSize(font_size);
                const Value &bg_colour(element->getProperties().find("bg_color"));
                if (bg_colour != SymbolTable::Null)
                    el->setBackgroundColor(colourFromProperty(element, "bg_color"));
                const Value &text_colour(element->getProperties().find("text_color"));
                if (text_colour != SymbolTable::Null)
                    el->setTextColor(colourFromProperty(element, "text_color"));
				const Value &alignment_v(element->getProperties().find("alignment"));
				if (alignment_v != SymbolTable::Null) el->setPropertyValue("Alignment", alignment_v.asString());
				const Value &valignment_v(element->getProperties().find("valign"));
				if (valignment_v != SymbolTable::Null) el->setPropertyValue("Vertical Alignment", valignment_v.asString());
				if (format_val != SymbolTable::Null) el->setValueFormat(format_val.asString());
				if (value_type != -1) el->setValueType(value_type);
				if (value_scale != 1.0) el->setValueScale( value_scale );
				if (tab_pos) el->setTabPosition(tab_pos);
				if (lp)
					lp->link(new LinkableText(el));
				if (visibility) el->setVisibilityLink(visibility);
				if (border != SymbolTable::Null) el->setBorder(border.iValue);
				el->setInvertedVisibility(ivis);
				el->setChanged(false);
			}
			if (kind == "IMAGE") {
				EditorImageView *el = new EditorImageView(s, window, element->getName(), lp, 0);
				el->setName(element->getName());
				el->setDefinition(element);
				if (connection != SymbolTable::Null) {
					// TODO: why is there no setRemoteName() here?
					el->setConnection(connection.asString());
				}
				const Value &img_scale_val(element->getProperties().find("scale"));
				double img_scale = 1.0f;
				if (img_scale_val != SymbolTable::Null) img_scale_val.asFloat(img_scale);
				fixElementPosition( el, element->getProperties());
				fixElementSize( el, element->getProperties());
				if (font_size) el->setFontSize(font_size);
				if (format_val != SymbolTable::Null) el->setValueFormat(format_val.asString());
				if (value_type != -1) el->setValueType(value_type);
				if (value_scale != 1.0) el->setValueScale( value_scale );
				el->setScale( img_scale );
				if (tab_pos) el->setTabPosition(tab_pos);
				el->setInvertedVisibility(ivis);
        const Value &image_file_v( (lp) ? lp->value() : (element->getProperties().find("image_file")));
				if (image_file_v != SymbolTable::Null) {
					std::string ifn = image_file_v.asString();
					el->setImageName(ifn);
					el->fit();
				}
				if (border != SymbolTable::Null) el->setBorder(border.iValue);
				if (lp) {
					lp->link(new LinkableText(el));
				if (visibility) el->setVisibilityLink(visibility);
				}
				el->setChanged(false);
			}
			if (kind == "PROGRESS") {
				EditorProgressBar *ep = new EditorProgressBar(s, window, element->getName(), lp);
				ep->setDefinition(element);
				fixElementPosition( ep, element->getProperties());
				fixElementSize( ep, element->getProperties());
				if (connection != SymbolTable::Null) {
					ep->setRemoteName(remote.asString());
					ep->setConnection(connection.asString());
				}
				if (format_val != SymbolTable::Null) ep->setValueFormat(format_val.asString());
				if (value_type != -1) ep->setValueType(value_type);
				if (value_scale != 1.0) ep->setValueScale( value_scale );
				if (tab_pos) ep->setTabPosition(tab_pos);
				if (border != SymbolTable::Null) ep->setBorder(border.iValue);
				ep->setInvertedVisibility(ivis);
				ep->setChanged(false);
				if (lp)
					lp->link(new LinkableNumber(ep));
				if (visibility) ep->setVisibilityLink(visibility);
			}
			if (kind == "TEXT") {
				EditorTextBox *textBox = new EditorTextBox(s, window, element->getName(), lp);
				textBox->setDefinition(element);
        const Value &text_v( (lp) ? lp->value() : (remote != SymbolTable::Null) ? "" : element->getProperties().find("text"));
				if (text_v != SymbolTable::Null) textBox->setValue(text_v.asString());
				const Value &alignment_v(element->getProperties().find("alignment"));
				if (alignment_v != SymbolTable::Null) textBox->setPropertyValue("Alignment", alignment_v.asString());
				const Value &valignment_v(element->getProperties().find("valign"));
				if (valignment_v != SymbolTable::Null) textBox->setPropertyValue("Vertical Alignment", valignment_v.asString());
				textBox->setEnabled(true);
				textBox->setEditable(true);
				if (connection != SymbolTable::Null) {
					textBox->setRemoteName(remote.asString());
					textBox->setConnection(connection.asString());
				}
				if (format_val != SymbolTable::Null) textBox->setValueFormat(format_val.asString());
				if (value_type != -1) textBox->setValueType(value_type);
				if (value_scale != 1.0) textBox->setValueScale( value_scale );
				fixElementPosition( textBox, element->getProperties());
				fixElementSize( textBox, element->getProperties());
				if (font_size) textBox->setFontSize(font_size);
				if (tab_pos) textBox->setTabPosition(tab_pos);
				if (border != SymbolTable::Null) textBox->setBorder(border.iValue);
				textBox->setName(element->getName());
				if (lp)
					textBox->setTooltip(remote.asString());
				else
					textBox->setTooltip(element->getName());
				EditorGUI *gui = this->gui;
				if (lp)
					lp->link(new LinkableText(textBox));
				if (visibility) textBox->setVisibilityLink(visibility);
				textBox->setInvertedVisibility(ivis);
				textBox->setChanged(false);
				textBox->setCallback( [textBox, gui](const std::string &value)->bool{
					if (!textBox->getRemote()) return true;
					const std::string &conn = textBox->getRemote()->group();
					char *rest = 0;
					{
						long val = strtol(value.c_str(),&rest,10);
						if (*rest == 0) {
							gui->queueMessage(conn,
									gui->getIODSyncCommand(conn,
									textBox->getRemote()->address_group(),
									textBox->getRemote()->address(), (int)(val * textBox->valueScale()) ),
								[](std::string s){std::cout << s << "\n"; });
							return true;
						}
					}
					{
						double val = strtod(value.c_str(),&rest);
						if (*rest == 0)  {
							gui->queueMessage(conn,
								gui->getIODSyncCommand(conn, textBox->getRemote()->address_group(),
									textBox->getRemote()->address(), (float)(val * textBox->valueScale())) , [](std::string s){std::cout << s << "\n"; });
							return true;
						}
					}
					return false;
				});
			}
			else if (kind == "PLOT" || (element_class &&element_class->getBase() == "PLOT") ) {
				EditorLinePlot *lp = new EditorLinePlot(s, window, element->getName(), nullptr);
				lp->setDefinition(element);
				lp->setBufferSize(gui->sampleBufferSize());
				fixElementPosition( lp, element->getProperties());
				fixElementSize( lp, element->getProperties());
				if (connection != SymbolTable::Null) {
					lp->setRemoteName(remote.asString());
					lp->setConnection(connection.asString());
				}
				if (format_val != SymbolTable::Null) lp->setValueFormat(format_val.asString());
				if (value_type != -1) lp->setValueType(value_type);
				if (value_scale != 1.0) lp->setValueScale( value_scale );
				if (font_size) lp->setFontSize(font_size);
				if (tab_pos) lp->setTabPosition(tab_pos);
        {
          bool should_overlay_plots;
          if (element->getProperties().find("overlay_plots").asBoolean(should_overlay_plots))
            lp->overlay(should_overlay_plots);
        }
				const Value &monitors(element->getProperties().find("monitors"));
				lp->setInvertedVisibility(ivis);
				if (monitors != SymbolTable::Null) {
					lp->setMonitors(this, monitors.asString());
				}
				lp->setChanged(false);
				if (visibility) lp->setVisibilityLink(visibility);
			}
			else if (kind == "BUTTON" || kind == "INDICATOR") {
				const Value &caption_v(element->getProperties().find("caption"));
				EditorButton *b = new EditorButton(s, window, element->getName(), lp,
												   (caption_v != SymbolTable::Null)?caption_v.asString(): element->getName());
				if (kind == "INDICATOR") b->setEnabled(false); else b->setEnabled(true);
				b->setDefinition(element);
				b->setBackgroundColor(colourFromProperty(element, "bg_color"));
				b->setTextColor(colourFromProperty(element, "text_colour"));
				b->setOnColor(colourFromProperty(element, "bg_on_color"));
				b->setOnTextColor(colourFromProperty(element, "on_text_colour"));
				b->setFlags(element->getIntProperty("behaviour", nanogui::Button::NormalButton));
				if (connection != SymbolTable::Null) {
					b->setRemoteName(remote.asString());
					b->setConnection(connection.asString());
				}
				fixElementPosition( b, element->getProperties());
				fixElementSize( b, element->getProperties());
				if (font_size) b->setFontSize(font_size);
				if (tab_pos) b->setTabPosition(tab_pos);
				if (border != SymbolTable::Null) b->setBorder(border.iValue);
				b->setInvertedVisibility(ivis);
				b->setWrap(wrap);
				EditorGUI *gui = this->gui;

				{
					const Value &caption_v = element->getProperties().find("caption");
					if (caption_v != SymbolTable::Null) b->setCaption(caption_v.asString());
				}
				{
					const Value &caption_v = element->getProperties().find("on_caption");
					if (caption_v != SymbolTable::Null) b->setOnCaption(caption_v.asString());
				}
				{
					const Value &cmd(element->getProperties().find("command"));
					if (cmd != SymbolTable::Null && cmd.asString().length()) b->setCommand(cmd.asString());
				}
				b->setupButtonCallbacks(lp, gui);
				if (visibility) b->setVisibilityLink(visibility);
				b->setChanged(false);
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

void UserWindow::load(const std::string &path_str) {

    boost::filesystem::path path(path_str);
    std::string extension_string = path.extension().string();
    std::list<std::string> files;
    if (extension_string.compare(".humid_project") == 0)
    {
        std::string folder_path = path.parent_path().string();
        files.push_back(folder_path);
    }
    else
    {
        files.push_back(path.string());
    }
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
		lp->update();
	}
}


void UserWindow::getPropertyNames(std::list<std::string> &names) {
	names.push_back("Screen Width");
	names.push_back("Screen Height");
	names.push_back("Screen Id");
	//names.push_back("Window Width");
	//names.push_back("Window Height");
	names.push_back("File Name");
}

void UserWindow::loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) {
	property_map["Screen Width"] = "screen_width";
	property_map["Screen Height"] = "screen_height";
	property_map["Screen Id"] = "screen_id";
	property_map["File Name"] = "file_name";
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
			 if (uw->structure()->getStructureDefinition()) {
			 		uw->structure()->getStructureDefinition()->setName(value);
					uw->structure()->setKind(value);
			 }
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
	return SymbolTable::Null;
}

CircularBuffer *UserWindow::getDataBuffer(const std::string item) {
	std::map<std::string, CircularBuffer *>::iterator found = data.find(item);
	if (found == data.end()) return nullptr;
	//	addDataBuffer(item, gui->sampleBufferSize());
	return (*found).second;
}

void UserWindow::refresh() { window->performLayout(gui->nvgContext()); }
