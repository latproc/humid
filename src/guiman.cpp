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
#if defined(_WIN32)
#include <windows.h>
#endif
#include <nanogui/glutil.h>
#include <iostream>
#include <fstream>
#include <locale>
#include <string>
#include "propertymonitor.h"
#include "draghandle.h"
#include "manuallayout.h"

#include <libgen.h>
#include <zmq.hpp>
#include <cJSON.h>
#include <MessageEncoding.h>
#include <MessagingInterface.h>
#include <signal.h>
#include <SocketMonitor.h>
#include <ConnectionManager.h>

#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/program_options.hpp>


using std::cout;
using std::cerr;
using std::endl;
using std::locale;
using nanogui::Vector2i;
using nanogui::Vector2d;
using nanogui::MatrixXd;
using nanogui::Matrix3d;

namespace po = boost::program_options;

const int DEBUG_ALL = 255;
#define DEBUG_BASIC ( 1 & debug)
static nanogui::DragHandle *drag_handle = 0;
int debug = DEBUG_ALL;
int saved_debug = 0;

/* Clockwork Interface */
const char *program_name;
enum ProgramState { s_initialising, s_running, s_finished } program_state = s_initialising;
boost::mutex update_mutex;
int cw_out = 5555;
std::string host("127.0.0.1");
const char *local_commands = "inproc://local_cmds";

static bool need_refresh = false; // not fully implemented yet

struct timeval start;

class SetupDisconnectMonitor : public EventResponder {

public:
	void operator()(const zmq_event_t &event_, const char* addr_) {
	}
};

class SetupConnectMonitor : public EventResponder {

public:
	void operator()(const zmq_event_t &event_, const char* addr_) {
		need_refresh = true;
	}
};

uint64_t get_diff_in_microsecs(struct timeval *now, struct timeval *then) {
	uint64_t t = (now->tv_sec - then->tv_sec);
	t = t * 1000000 + (now->tv_usec - then->tv_usec);
	return t;
}

/* Drag and Drop interface */
#define TO_STRING( ID ) #ID

static nanogui::Screen *screen = 0;
static nanogui::Widget *mainWindow = 0;

std::ostream &operator<<(std::ostream& out, Handle::Mode m) {
	switch (m) {

	case Handle::NONE:
		out << TO_STRING(NONE);
		break;

	case Handle::POSITION:
		out << TO_STRING(POSITION);
		break;

	case Handle::RESIZE_TL:
		out << TO_STRING(RESIZE_TL);
		break;

	case Handle::RESIZE_T:
		out << TO_STRING(RESIZE_T);
		break;

	case Handle::RESIZE_TR:
		out << TO_STRING(RESIZE_TR);
		break;

	case Handle::RESIZE_R:
		out << TO_STRING(RESIZE_R);
		break;

	case Handle::RESIZE_BL:
		out << TO_STRING(RESIZE_BL);
		break;

	case Handle::RESIZE_L:
		out << TO_STRING(RESIZE_L);
		break;

	case Handle::RESIZE_BR:
		out << TO_STRING(RESIZE_BR);
		break;

	case Handle::RESIZE_B:
		out << TO_STRING(RESIZE_B);
		break;
	}

	return out;
}

//std::vector<Handle>handles(9);
Handle::Mode all_handles[] = {
	Handle::POSITION,
	Handle::RESIZE_TL,
	Handle::RESIZE_T,
	Handle::RESIZE_TR,
	Handle::RESIZE_R,
	Handle::RESIZE_BL,
	Handle::RESIZE_L,
	Handle::RESIZE_BR,
	Handle::RESIZE_B
};

// ref http://stackoverflow.com/questions/7302996/changing-the-delimiter-for-cin-c

struct comma_is_space : std::ctype<char> {
	comma_is_space() : std::ctype<char>(get_table()) {}

	static mask const* get_table() {
		static mask rc[table_size];
		rc[','] = std::ctype_base::space;
		return &rc[0];
	}
};

// Selection - provides an identification for the currently selected object with
// drawing functions for edit and run modes
class WidgetSelector {
	public:
		enum Mode { MODE_EDIT, MODE_RUN};
		
		WidgetSelector() : mode(MODE_EDIT), current(0) {}
		WidgetSelector(Mode m) : mode(m), current(0) {}
		
		void select(nanogui::Widget *select) { 
			current = select; 
			select->requestFocus();
		}
		nanogui::Widget *selected() const { return current; }
		void draw(NVGcontext *ctx) {
			using namespace nanogui;
			if (!current) { std::cout << "not current\n"; return; }
			cout << "drawing selected rect\n";
			nvgBeginPath(ctx);
			nvgStrokeWidth(ctx, 1.0f);
			nvgRoundedRect(ctx, current->position().x()+1 , current->position().y()+1 , 
					current->size().x() + 1, current->size().y() + 1, current->theme()->mButtonCornerRadius);
			nvgStrokeColor(ctx, nvgRGBA(60,200,0,64));
			nvgStroke(ctx);
		}
	
	private:
		Mode mode;
		nanogui::Widget *current;
};

// WindowStagger - provides an automatic position for the next window
class WindowStagger {
	public:
		WindowStagger(const nanogui::Screen *s) : screen(s), next_pos(20,40), stagger(20,20) {}
		WindowStagger(const nanogui::Vector2i &stag) : next_pos(40,20), stagger(stag) {}
		
		nanogui::Vector2i pos() {
			nanogui::Vector2i pos(next_pos);
			next_pos += stagger;
			if (next_pos.x() > screen->size().x() - 100)
				next_pos.x() = 20;
			if (next_pos.y() > screen->size().y() - 100)
				next_pos.y() = 40;
			return pos;
		}
		
	private:
		const nanogui::Screen *screen;
		nanogui::Vector2i next_pos;
		nanogui::Vector2i stagger;	
};

class Editor {

public:
	static Editor *instance() {
		if (_instance==0) _instance = new Editor();

		return _instance;
	}

	void setEditMode(bool which) {
		mEditMode = which;
		if (!which && widget_selector) widget_selector->select(0);
	}

	bool isEditMode() {
		return mEditMode;
	}

	bool isCreateMode() {
		return mCreateMode;
	}

	void setCreateMode(bool which) {
		mCreateMode = which;
	}

	void refresh(bool ) {
		if (screen) {
			if (mainWindow ) {
				nanogui::Theme *theme = mainWindow->theme();
				theme->incRef();
				mainWindow->setTheme(new nanogui::Theme(screen->nvgContext()));
				mainWindow->setTheme(theme);
				theme->decRef();
			}

			screen->performLayout();

		}
	}
	
	void setSelector( WidgetSelector* sel) { widget_selector = sel; }
	WidgetSelector *selector() { return widget_selector; }

private:
	Editor() :mEditMode(false), mCreateMode(false) {

	}

	static Editor *_instance;
	bool mEditMode;
	bool mCreateMode;
	WidgetSelector *widget_selector;
};

#define EDITOR Editor::instance()

Editor *Editor::_instance = 0;

class EditorWidget {

public:

	EditorWidget() : dh(0), handles(9), handle_coordinates(9,2) { }

	virtual bool editorMouseButtonEvent(nanogui::Widget *widget, const nanogui::Vector2i &p, int button, bool down, int modifiers) {

		using namespace nanogui;

		if (EDITOR->isEditMode()) {
			return false;
		}
		else {
			if (drag_handle) drag_handle->setVisible(false);
		}

		return true; // caller should continue to call the default handler for the object
	}

	virtual bool editorMouseMotionEvent(nanogui::Widget *widget, const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) {
		if ( !EDITOR->isEditMode() )
			return true; // caller should continue to call the default handler for the object

		nanogui::Vector2d pt(p.x(), p.y());

		nanogui::VectorXd distances = (handle_coordinates.rowwise() - pt.transpose()).rowwise().squaredNorm();

		double min = distances.row(0).x();

		int idx = 0;

		for (int i=1; i<9; ++i) {
			if (distances.row(i).x() < min) {
				min = distances.row(i).x();
				idx = i;
			}
		}

		if (min >= 16) {
			drag_handle->setVisible(false);
			updateHandles(widget);
			return false;
		}

		std::cout << "smallest distance: " << min << " " << handles[idx].mode() << "\n";

		if (handles[idx].mode() != Handle::NONE) {
			drag_handle->setTarget(widget);

			drag_handle->setPosition( Vector2i(handles[idx].position().x() - drag_handle->size().x()/2,
			                                   handles[idx].position().y() - drag_handle->size().y()/2) ) ;

			if (drag_handle->propertyMonitor())
				drag_handle->propertyMonitor()->setMode( handles[idx].mode() );

			drag_handle->setVisible(true);
		}
		else {
			drag_handle->setVisible(false);
			updateHandles(widget);
		}

		return false;
	}

	virtual bool editorMouseEnterEvent(nanogui::Widget *widget, const Vector2i &p, bool enter) {
		if (enter) updateHandles(widget);

		return true;
	}

	void updateHandles(nanogui::Widget *w) {
		for (int i=0; i<9; ++i) {
			Handle h = Handle::create(all_handles[i], w->position(), w->size());
			handle_coordinates(i,0) = h.position().x();
			handle_coordinates(i,1) = h.position().y();
			handles[i] = h;
		}
	}

	nanogui::DragHandle *dh;
	std::vector<Handle> handles;
	MatrixXd handle_coordinates;
};



class EditorButton : public nanogui::Button, public EditorWidget {

public:

	EditorButton(Widget *parent, const std::string &caption = "Untitled", int modbus_address = 0, int icon = 0)
			: Button(parent, caption, icon), dh(0), handles(9), handle_coordinates(9,2), addr(modbus_address) {
	}

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override {

		using namespace nanogui;

		if (editorMouseButtonEvent(this, p, button, down, modifiers))
			return nanogui::Button::mouseButtonEvent(p, button, down, modifiers);
		else
			if (down && EDITOR->selector()) EDITOR->selector()->select(this);


		return true;
	}

	virtual bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override {

		if (editorMouseMotionEvent(this, p, rel, button, modifiers))
			return Button::mouseMotionEvent(p, rel, button, modifiers);

		return true;
	}

	virtual bool mouseEnterEvent(const Vector2i &p, bool enter) override {

		if (editorMouseEnterEvent(this, p, enter))
			return Button::mouseEnterEvent(p, enter);

		return true;
	}

	int address() const {
		return addr;
	}

	nanogui::DragHandle *dh;
	std::vector<Handle> handles;
	MatrixXd handle_coordinates;
	int addr;
};


template<typename T>
class Selectable : public T {
	
}; 
template<>
class Selectable<EditorButton> : public EditorButton {
public:
	Selectable(nanogui::Widget *parent, const std::string &caption = "Untitled", 
		int modbus_address = 0, int icon = 0) : EditorButton(parent, caption, modbus_address, icon), mSelected(false) {
			
	}
	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override {

		using namespace nanogui;

		if (editorMouseButtonEvent(this, p, button, down, modifiers))
			return nanogui::Button::mouseButtonEvent(p, button, down, modifiers);
		else
			if (down && EDITOR->selector()) //EDITOR->selector()->select(this);
				mSelected = !mSelected;
		return true;
	}
	virtual void draw(NVGcontext *ctx) override {
		if (mSelected) {
			nvgStrokeWidth(ctx, 1.0f);
			nvgBeginPath(ctx);
			nvgRect(ctx, mPos.x() - 0.5f, mPos.y() - 0.5f, mSize.x() + 1, mSize.y() + 1);
			nvgStrokeColor(ctx, nvgRGBA(60, 200, 0, 255));
			nvgStroke(ctx);
		}

		EditorButton::draw(ctx);
	}
private:
	bool mSelected;  
};


class ClockworkExample : public nanogui::Screen {

public:
	static const bool kRESIZEABLE = true;
	static const bool kNOTRESIZEABLE = false;
	static const bool kFULLSCREEN = true; // breaks mouse interaction atm
	static const bool kWINDOWED = false;

public:
	ClockworkExample() : nanogui::Screen(nanogui::Vector2i(1024, 768), "NanoGUI Test", kRESIZEABLE, kWINDOWED),
			window(0), subscription_manager(0), disconnect_responder(0), connect_responder(0),
			iosh_cmd(0), cmd_interface(0), next_device_num(0), next_state_num(0),scale(1000),
			window_stagger(this) {

		using namespace nanogui;

		Window *toolbar = new Window(this, "Toolbar");
		ToolButton *tb = new ToolButton(toolbar, ENTYPO_ICON_PENCIL);
		tb->setFlags(Button::ToggleButton);
		tb->setFixedSize(Vector2i(32,32));
		tb->setChangeCallback([](bool state) {
			EDITOR->setEditMode(state);
		}

		                     );
		tb = new ToolButton(toolbar, ENTYPO_ICON_NEW);
		tb->setFlags(Button::ToggleButton);
		tb->setTooltip("New");
		tb->setFixedSize(Vector2i(32,32));
		tb = new ToolButton(toolbar, ENTYPO_ICON_INSTALL);
		tb->setTooltip("Refresh");
		tb->setFixedSize(Vector2i(32,32));
		tb->setChangeCallback([](bool state) {
			if (state) EDITOR->refresh(state);
		}

		                     );
		ToolButton *settings_button = new ToolButton(toolbar, ENTYPO_ICON_COG);
		settings_button->setFixedSize(Vector2i(32,32));
		settings_button->setTooltip("Theme settings");
		tb = new ToolButton(toolbar, ENTYPO_ICON_LAYOUT);
		tb->setTooltip("Create");
		tb->setFixedSize(Vector2i(32,32));
		tb->setChangeCallback([](bool state) {
			EDITOR->setCreateMode(state);
		});

		BoxLayout *bl = new BoxLayout(Orientation::Horizontal);
		toolbar->setLayout(bl);

		Window *palette = new Window(this, "Object Palette");
		palette->setFixedSize(Vector2i(250, 600));
		palette->setPosition( Vector2i(650,15));
		VScrollPanel *palette_scroller = new VScrollPanel(palette);
		palette_scroller->setFixedSize(Vector2i(240, 600 - palette->theme()->mWindowHeaderHeight));
		palette_scroller->setPosition( Vector2i(5, palette->theme()->mWindowHeaderHeight+1));
		Widget *palette_content = new Widget(palette_scroller);
		palette_content->setLayout(new ManualLayout(palette->size()));
		//new Label(palette_scroller, "Test", "sans-bold");
		window = createPanelPage("Panel Screen", "CLOCKWORK_tags.csv", palette_content);
		Widget *container = window;
		GridLayout *palette_layout = new GridLayout(Orientation::Horizontal,1);
		palette_layout->setSpacing(4);
		palette_layout->setMargin(4);
		palette_content->setLayout(palette_layout);
		palette_scroller->setFixedSize(Vector2i(240,
			600 - palette->theme()->mWindowHeaderHeight));

		Window * window2 = createPanelPage("Panel 2");
		window2->setTheme(new nanogui::Theme(mNVGContext));
		window2->setLayout(new ManualLayout(size()));

		drag_handle = new DragHandle(window, new PositionMonitor);
		mainWindow = window;

		FormHelper *properties = new FormHelper(this);
		property_window = properties->addWindow(nanogui::Vector2i(10, 10), "Theme Properties");
		//window->setPosition(Vector2i(500,20));
		window->setTheme(new nanogui::Theme(mNVGContext));
		properties->addVariable("Standard Font Size", window->theme()->mStandardFontSize);
		properties->addVariable("Button Font Size", window->theme()->mButtonFontSize);
		properties->addVariable("TextBox Font Size", window->theme()->mTextBoxFontSize);
		properties->addVariable("Corner Radius", window->theme()->mWindowCornerRadius);
		properties->addVariable("Header Height", window->theme()->mWindowHeaderHeight);
		properties->addVariable("Drop Shadow Size", window->theme()->mWindowDropShadowSize);
		properties->addVariable("Button Corner Radius", window->theme()->mButtonCornerRadius);
		properties->addVariable("Drop Shadow Colour", window->theme()->mDropShadow);
		properties->addVariable("Transparent Colour", window->theme()->mTransparent);
		properties->addVariable("Dark Border Colour", window->theme()->mBorderDark);
		properties->addVariable("Light Border Colour", window->theme()->mBorderLight);
		properties->addVariable("Medium Border Colour", window->theme()->mBorderMedium);
		properties->addVariable("Text Colour", window->theme()->mTextColor);
		properties->addVariable("Disabled Text Colour", window->theme()->mDisabledTextColor);
		properties->addVariable("Text Shadow Colour", window->theme()->mTextColorShadow);
		properties->addVariable("Icon Colour", window->theme()->mIconColor);
		properties->addVariable("Focussed Btn Gradient Top Colour", window->theme()->mButtonGradientTopFocused);
		properties->addVariable("Focussed Btn Bottom Colour", window->theme()->mButtonGradientBotFocused);
		properties->addVariable("Btn Gradient Top Colour", window->theme()->mButtonGradientTopUnfocused);
		properties->addVariable("Btn Gradient Bottom Colour", window->theme()->mButtonGradientBotUnfocused);
		properties->addVariable("Pushed Btn Top Colour", window->theme()->mButtonGradientTopPushed);
		properties->addVariable("Pushed Btn Bottom Colour", window->theme()->mButtonGradientBotPushed);
		properties->addVariable("Window Colour", window->theme()->mWindowFillUnfocused);
		properties->addVariable("Focussed Win Colour", window->theme()->mWindowFillFocused);
		properties->addVariable("Window Title Colour", window->theme()->mWindowTitleUnfocused);
		properties->addVariable("Focussed Win Title Colour", window->theme()->mWindowTitleFocused);

		performLayout(mNVGContext);
		property_window->setVisible(false);

		settings_button->setChangeCallback([this](bool state) {
			property_window->incRef();
			property_window->setVisible(!property_window->visible());
		}

		                                  );

		window->setLayout(new ManualLayout(size()));
		screen = this;
		EDITOR->setSelector(&widget_selector);
		
		mShader.init(
			/* An identifying name */
			"a_simple_shader",

			/* Vertex shader */
			"#version 330\n"
			"uniform mat4 modelViewProj;\n"
			"in vec3 position;\n"
			"void main() {\n"
			"    gl_Position = modelViewProj * vec4(position, 1.0);\n"
			"}",

			/* Fragment shader */
			"#version 330\n"
			"out vec4 color;\n"
			"uniform float intensity;\n"
			"void main() {\n"
			"    color = vec4(vec3(intensity), 1.0);\n"
			"}"
		);

		MatrixXu indices(3, 2); /* Draw 2 triangles */
		indices.col(0) << 0, 1, 2;
		indices.col(1) << 2, 3, 0;

		MatrixXf positions(3, 4);
		positions.col(0) << -1, -1, 0;
		positions.col(1) <<  1, -1, 0;
		positions.col(2) <<  1,  1, 0;
		positions.col(3) << -1,  1, 0;

		mShader.bind();
		mShader.uploadIndices(indices);
		mShader.uploadAttrib("position", positions);
		mShader.setUniform("intensity", 0.5f);
		
	}

	nanogui::Window *createPanelPage(const char *name, const char *filename = 0, nanogui::Widget *palette_content = 0) {

		using namespace nanogui;
		Window *window = new Window(this, name);
		window->setPosition(window_stagger.pos());
		window->setFixedSize(Vector2i(640, 480));

		if (filename && palette_content) {
			std::string modbus_settings(filename);
			std::ifstream init(modbus_settings);

			if (init.good()) {
				GridLayout *layout = new GridLayout(Orientation::Horizontal,6);
				window->setLayout(layout);
				importModbusInterface(init, palette_content, window);
			}
		}

		return window;
	}

	bool importModbusInterface(std::istream &init, Widget *palette_content, Widget *container) {

		using namespace nanogui;

		if (!init.good()) return false;

		char buf[200];

		std::istringstream iss;

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

			iss >> protocol_id >> device_name >> tag_name >> data_type
			>> data_count >> retentive >> address_str >> array_start
			>> array_end;
			char kind = address_str[1];
			{
				Selectable<EditorButton> *b = new Selectable<EditorButton>(palette_content, tag_name, 0);
				b->setEnabled(false);
				b->setFixedSize(Vector2i(200, 30));
				//b->setPosition(Vector2i(100,++pallete_row * 32));
			}

			switch (kind) {

			case '0': {
				size_t p = tag_name.find(".cmd_");

				if (p != std::string::npos) {
					std::string name = tag_name.substr(0,p);
					new Label(container, name, "sans-bold");
					char *rest = 0;
					int address_int = (int)strtol(address_str.c_str()+2,&rest, 10);

					if (*rest) {
						std::cerr << "Unexpected data in address\n";
						address_int = 0;
					}

					EditorButton *b = new EditorButton(container, tag_name.substr(p+5), address_int);

					b->setCallback([b, this] {
						this->queueMessage(getIODSyncCommand(0, b->address(), 1));
						this->queueMessage(getIODSyncCommand(0, b->address(), 0));
					});
				}
				else {
					new Label(container, tag_name, "sans-bold");
					EditorButton *b = new EditorButton(container);
					b->setFixedSize(Vector2i(60, 25));
					b->setFlags(Button::ToggleButton);
					b->setChangeCallback([this,b](bool state) {
					this->queueMessage(getIODSyncCommand(0, b->address(), (state)?1:0));
					if (state)
						b->setBackgroundColor(Color(255, 80, 0, 255));
					else
						b->setBackgroundColor(b->theme()->mButtonGradientBotUnfocused   );
					});

				}
			}

			break;

			case '1': {
				new Label(container, tag_name, "sans-bold");
				CheckBox *b = new CheckBox(container, tag_name);
				//img->setPolicy(ImageView::SizePolicy::Expand);
				//b->setBackgroundColor(Color(200, 200, 200, 255));
				//b->setFixedSize(Vector2i(25, 25));
			}

			break;

			case '3': {
				new Label(container, tag_name, "sans-bold");
				TextBox *textBox = new TextBox(container);
				textBox->setFixedSize(Vector2i(60, 25));
				textBox->setValue("sample");
				textBox->setEnabled(true);
			}

			break;

			case '4': {
				new Label(container, tag_name, "sans-bold");
				auto textBox = new IntBox<int>(container);
				textBox->setFixedSize(Vector2i(60, 25));
				textBox->setEditable(true);
				textBox->setFixedSize(Vector2i(100, 20));
				textBox->setValue(50);
				//textBox->setUnits("Mhz");
				textBox->setDefaultValue("0");
				//textBox->setFontSize(16);
				textBox->setFormat("[1-9][0-9]*");
				textBox->setSpinnable(true);
				textBox->setMinValue(1);
				textBox->setValueIncrement(2);
			}

			break;

			default:
				;
			}

			cout << kind << " " << tag_name << " "

			     << data_type << " " << data_count << " " << address_str
			     << "\n";
		}
		//palette_content->setFixedSize( nanogui::Vector2i( (pallete_row+1) * 32, palette_content->fixedWidth()));

		return true;
	}

	nanogui::ref<nanogui::Window> getProperties() {
		return property_window;
	}

	~ClockworkExample() {
		mShader.free();
	}

	virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) override {

		if (Screen::keyboardEvent(key, scancode, action, modifiers))
			return true;

		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			setVisible(false);
			return true;
		}

		return false;
	}

	virtual void draw(NVGcontext *ctx) override {
		/* Draw the user interface */
		Screen::draw(ctx);
		
	}

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override {

		using namespace nanogui;

		if (!window->contains(p - window->position()))
			return Screen::mouseButtonEvent(p, button, down, modifiers);

		if (drag_handle && EDITOR->isCreateMode() && down) {
			//EDITOR->setCreateMode(false);
			drag_handle->incRef();
			removeChild(drag_handle);
			PropertyMonitor *pm = drag_handle->propertyMonitor();
			drag_handle->setPropertyMonitor(0);

			EditorButton *b = new EditorButton(window, "Untitled", 0);
			//b->setEnabled(false);
			//img->setPolicy(ImageView::SizePolicy::Expand);
			b->setBackgroundColor(Color(200, 30, 30, 255));
#if 0
			Screen*scrn = dynamic_cast<Screen*> (window->parent());

			if (scrn) {
				b->performLayout(scrn->nvgContext());
				b->draw(scrn->nvgContext());
			}

#endif
			b->setFixedSize(Vector2i(60, 40));
			b->setSize(Vector2i(60, 40));

			b->setPosition(p-window->position() - b->size()/2);
			window->addChild(drag_handle);
			performLayout();

			drag_handle->setPropertyMonitor(pm);

			std::cout << "drag handle refs: " << drag_handle->getRefCount() << "\n";

			drag_handle->decRef();

			return true;
		}
		else return Screen::mouseButtonEvent(p, button, down, modifiers);
	}

	virtual void drawContents() override {
		{
			
			using namespace nanogui;

			/* Draw the window contents using OpenGL */
			mShader.bind();

			Matrix4f mvp;
			mvp.setIdentity();
			mvp.topLeftCorner<3,3>() = Matrix3f(nanogui::AngleAxisf((float) glfwGetTime(),  Vector3f::UnitZ())) * 0.25f;

			mvp.row(0) *= (float) mSize.y() / (float) mSize.x();

			mShader.setUniform("modelViewProj", mvp);

			/* Draw 2 triangles starting at index 0 */
			mShader.drawIndexed(GL_TRIANGLES, 0, 2);
			boost::mutex::scoped_lock lock(update_mutex);

			if (program_state == s_initialising) {

				std::cout << "-------- Starting Command Interface ---------\n" << std::flush;
				std::cout << "connecting to clockwork on " << host << ":" << cw_out << "\n";
				g_iodcmd = MessagingInterface::create(host, cw_out);
				g_iodcmd->start();

				iosh_cmd = new zmq::socket_t(*MessagingInterface::getContext(), ZMQ_REP);
				iosh_cmd->bind(local_commands);
				usleep(100);

				subscription_manager = new SubscriptionManager("PANEL_CHANNEL", eCLOCKWORK, "localhost", 5555);
				subscription_manager->configureSetupConnection(host.c_str(), cw_out);
				disconnect_responder = new SetupDisconnectMonitor;
				connect_responder = new SetupConnectMonitor;
				subscription_manager->monit_setup->addResponder(ZMQ_EVENT_DISCONNECTED, disconnect_responder);
				subscription_manager->monit_setup->addResponder(ZMQ_EVENT_CONNECTED, connect_responder);
				subscription_manager->setupConnections();
				program_state = s_running;
			}
			else if (program_state == s_running) {
				if (!cmd_interface) {
					cmd_interface = new zmq::socket_t(*MessagingInterface::getContext(), ZMQ_REQ);
					cmd_interface->connect(local_commands);
				}

				static bool reported_error = false;

				zmq::pollitem_t items[] = {
					{ subscription_manager->setup(), 0, ZMQ_POLLIN, 0 },
					{ subscription_manager->subscriber(), 0, ZMQ_POLLIN, 0 },
					{ *iosh_cmd, 0, ZMQ_POLLIN, 0 }
				};

				try {
					{
						int item = 1;

						try {
							int sock_type = 0;
							size_t param_size = sizeof(sock_type);
							subscription_manager->setup().getsockopt(ZMQ_TYPE, &sock_type, &param_size);
							++item;
							subscription_manager->subscriber().getsockopt(ZMQ_TYPE, &sock_type, &param_size);
							++item;
							iosh_cmd->getsockopt(ZMQ_TYPE, &sock_type, &param_size);

						}
						catch (zmq::error_t zex) {
							std::cerr << "zmq exception " << zmq_errno()  << " " << zmq_strerror(zmq_errno())
							          << " for item " << item << " in SubscriptionManager::checkConnections\n";

						}
					}

					if (!subscription_manager->checkConnections(items, 3, *iosh_cmd)) {
						if (debug) {
							std::cout << "no connection to iod\n";
							reported_error = true;
						}
					}
					else {
						/*
											if (!sendMessage(cmd.c_str(), *this->cmd_interface, response, cmd_timeout))
											{
												std::cerr << program_name << "Message send of " << cmd << " failed\n";
											}
						*/
						zmq::message_t update;

						if (!subscription_manager->subscriber().recv(&update, ZMQ_DONTWAIT)) {
							if (errno == EAGAIN) {
								usleep(50);
							}
							else if (errno == EFSM) exit(1);
							else if (errno == ENOTSOCK) exit(1);
							else if (errno == ETIMEDOUT) {} else std::cerr << "subscriber recv: " << zmq_strerror(zmq_errno()) << "\n";
						}
						else {

							struct timeval now;
							gettimeofday(&now, 0);
							std::ostream &output(std::cout);
							long len = update.size();
							char *data = (char *)malloc(len+1);
							memcpy(data, update.data(), len);
							data[len] = 0;

							if (DEBUG_BASIC) std::cout << "received: "<<data<<" from clockwork\n";

							std::list<Value> *message = 0;

							std::string machine, property, op, state;

							Value val(SymbolTable::Null);

							if (MessageEncoding::getCommand(data, op, &message)) {
								if (op == "STATE"&& message->size() == 2) {
									machine = message->front().asString();
									message->pop_front();
									state = message->front().asString();
									int state_num = lookupState(state);
									std::map<std::string, int>::iterator idx = device_map.find(machine);

									if (idx == device_map.end()) //new machine
										device_map[machine] = next_device_num++;

									output << get_diff_in_microsecs(&now, &start)/scale
									<< "\t" << machine << "\t" << state << "\t" << state_num;
								}
								else if (op == "UPDATE") {

									//output << get_diff_in_microsecs(&now, &start)/scale << data << "\n";
									output << get_diff_in_microsecs(&now, &start)/scale;
									std::list<Value>::iterator iter = message->begin();

									while (iter!= message->end()) {
										const Value &v =  *iter++;
										output << v;

										if (iter != message->end()) output << "\t";
									}
								}
								else if (op == "PROPERTY" && message->size() == 3) {
									std::string machine = message->front().asString();
									message->pop_front();
									std::string prop = message->front().asString();
									message->pop_front();
									property = machine + "." + prop;
									val = message->front();

									std::map<std::string, int>::iterator idx = device_map.find(property);

									if (idx == device_map.end()) //new machine
										device_map[property] = next_device_num++;

									output << get_diff_in_microsecs(&now, &start)/ scale
									<< "\t" << property << "\tvalue\t";

									if (val.kind == Value::t_string)
										output << "\"" << escapeNonprintables(val.asString().c_str()) << "\"";
									else
										output << escapeNonprintables(val.asString().c_str());
								}
							}

							free(data);

							output << "\n";
						}
					}
				}
				catch (zmq::error_t zex) {
					if (zmq_errno() != EINTR) {
						std::cerr << "zmq exception " << zmq_errno()  << " "
						          << zmq_strerror(zmq_errno()) << " polling connections\n";
					}
				}
				catch (std::exception ex) {
					std::cerr << "polling connections: " << ex.what() << "\n";
				}
			}
		}
	}

	int lookupState(std::string &state) {
		int state_num = 0;
		std::map<std::string, int>::iterator idx = state_map.find(state);

		if (idx == state_map.end()) { // new state
			state_num = next_state_num;
			state_map[state] = next_state_num++;
		}
		else
			state_num = (*idx).second;

		return state_num;
	}

	std::string escapeNonprintables(const char *buf) {
		const char *hex = "0123456789ABCDEF";
		std::string res;

		while (*buf) {
			if (isprint(*buf)) res += *buf;
			else if (*buf == '\015') res += "\\r";
			else if (*buf == '\012') res += "\\n";
			else if (*buf == '\010') res += "\\t";
			else {
				const char tmp[3] = { hex[ (*buf & 0xf0) >> 4], hex[ (*buf & 0x0f)], 0 };
				res = res + "#{" + tmp + "}";
			}

			++buf;
		}

		return res;
	}

	void queueMessage(std::string s) {
		messages.push_back(s);
	}

	void queueMessage(const char *s) {
		messages.push_back(s);
	}

	char *sendIOD(int group, int addr, int new_value);
	char *sendIODMessage(const std::string &s);
	std::string getIODSyncCommand(int group, int addr, bool which);
	std::string getIODSyncCommand(int group, int addr, int new_value);
	std::string getIODSyncCommand(int group, int addr, unsigned int new_value);

protected:
	nanogui::Window *window;
	SubscriptionManager *subscription_manager;
	SetupDisconnectMonitor *disconnect_responder;
	SetupConnectMonitor *connect_responder;
	zmq::socket_t *iosh_cmd;
	zmq::socket_t *cmd_interface;
	std::list<std::string> messages; // outgoing messages
	nanogui::ref<nanogui::Window> property_window;

	MessagingInterface *g_iodcmd;

	std::map<std::string, int> state_map;
	std::map<std::string, int> device_map;

	int next_device_num;
	int next_state_num;

	struct timeval start;
	long scale;
	WindowStagger window_stagger;
	WidgetSelector widget_selector;
	nanogui::GLShader mShader;

};


static void finish(int sig) {

	struct sigaction sa;
	sa.sa_handler = SIG_DFL;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGTERM, &sa, 0);
	sigaction(SIGINT, &sa, 0);
	sigaction(SIGUSR1, &sa, 0);
	sigaction(SIGUSR2, &sa, 0);
	program_state = s_finished;
}

static void toggle_debug(int sig) {
	if (debug && debug != saved_debug) saved_debug = debug;

	if (debug) debug = 0;
	else {
		if (saved_debug==0) saved_debug = 1;

		debug = saved_debug;
	}
}

static void toggle_debug_all(int sig) {
	if (debug) debug = 0;
	else debug = DEBUG_ALL;
}

bool setup_signals() {

	struct sigaction sa;
	sa.sa_handler = finish;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGTERM, &sa, 0) || sigaction(SIGINT, &sa, 0)) {
		return false;
	}

	sa.sa_handler = toggle_debug;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGUSR1, &sa, 0) ) {
		return false;
	}

	sa.sa_handler = toggle_debug_all;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGUSR2, &sa, 0) ) {
		return false;
	}

	return true;
}

std::string ClockworkExample::getIODSyncCommand(int group, int addr, bool which) {
	int new_value = (which) ? 1 : 0;
	char *msg = MessageEncoding::encodeCommand("MODBUS", group, addr, new_value);
	sendIODMessage(msg);

	if (DEBUG_BASIC) std::cout << "IOD command: " << msg << "\n";

	std::string s(msg);

	free(msg);

	return s;
}

std::string ClockworkExample::getIODSyncCommand(int group, int addr, int new_value) {
	char *msg = MessageEncoding::encodeCommand("MODBUS", group, addr, new_value);
	sendIODMessage(msg);

	if (DEBUG_BASIC) std::cout << "IOD command: " << msg << "\n";

	std::string s(msg);

	free(msg);

	return s;
}

std::string ClockworkExample::getIODSyncCommand(int group, int addr, unsigned int new_value) {
	char *msg = MessageEncoding::encodeCommand("MODBUS", group, addr, new_value);
	sendIODMessage(msg);

	if (DEBUG_BASIC) std::cout << "IOD command: " << msg << "\n";

	std::string s(msg);

	free(msg);

	return s;
}

char *ClockworkExample::sendIOD(int group, int addr, int new_value) {
	std::string s(getIODSyncCommand(group, addr, new_value));

	if (g_iodcmd)
		return g_iodcmd->send(s.c_str());
	else {
		if (DEBUG_BASIC) std::cout << "IOD interface not ready\n";

		return strdup("IOD interface not ready\n");
	}
}

char *ClockworkExample::sendIODMessage(const std::string &s) {
	std::cout << "sending " << s << "\n";

	if (g_iodcmd)
		return g_iodcmd->send(s.c_str());
	else {
		if (DEBUG_BASIC) std::cout << "IOD interface not ready\n";

		return strdup("IOD interface not ready\n");
	}
}

int main(int argc, const char ** argv ) {
	char *pn = strdup(argv[0]);
	program_name = strdup(basename(pn));
	free(pn);

	zmq::context_t context;
	MessagingInterface::setContext(&context);

	int cw_port;
	std::string hostname;

	program_state = s_initialising;
	setup_signals();

	po::options_description desc("Allowed options");
	desc.add_options()
	("help", "produce help message")
	("debug",po::value<int>(&debug)->default_value(0), "set debug level")
	("host", po::value<std::string>(&hostname)->default_value("localhost"),"remote host (localhost)")
	("cwout",po::value<int>(&cw_port)->default_value(5555), "clockwork outgoing port (5555)")
	("cwin", "clockwork incoming port (deprecated)")
	;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) {
		std::cout << desc << "\n";
		return 1;
	}

	if (vm.count("cwout")) cw_out = vm["cwout"].as<int>();

	if (vm.count("host")) host = vm["host"].as<std::string>();

	if (vm.count("debug")) debug = vm["debug"].as<int>();

	if (DEBUG_BASIC) std::cout << "Debugging\n";

	gettimeofday(&start, 0);

	try {
		nanogui::init();

		{
			nanogui::ref<ClockworkExample> app = new ClockworkExample();
			//app->drawAll();
			app->setVisible(true);

			nanogui::mainloop();
		}

		nanogui::shutdown();
	}
	catch (const std::runtime_error &e) {
		std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
#if defined(_WIN32)
		MessageBoxA(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
#else
		std::cerr << error_msg << endl;
#endif
		return -1;
	}

	return 0;
}



