#include <nanogui/screen.h>
#include <nanogui/vscrollpanel.h>

#include "propertyformhelper.h"
#include "propertyformwindow.h"

PropertyFormHelper::PropertyFormHelper(nanogui::Screen *screen) 
: nanogui::FormHelper(screen), mContent(0) { }

void PropertyFormHelper::clear() {
	using namespace nanogui;
	while (window()->childCount()) {
		window()->removeChild(0);
	}
}

nanogui::Window *PropertyFormHelper::addWindow(const nanogui::Vector2i &pos,
						  const std::string &title) {
	assert(mScreen);
	using namespace nanogui;
	if (mWindow) { mWindow->decRef(); mWindow = 0; }
	PropertyFormWindow *pfw = new PropertyFormWindow(mScreen, title);
	mWindow = pfw;
	mWindow->setSize(Vector2i(320, 640));
	mWindow->setFixedSize(Vector2i(320, 640));
	nanogui::VScrollPanel *palette_scroller = new nanogui::VScrollPanel(mWindow);
	palette_scroller->setSize(Vector2i(mWindow->width(), mWindow->height() - mWindow->theme()->mWindowHeaderHeight));
	palette_scroller->setFixedSize(Vector2i(mWindow->width(), mWindow->height() - mWindow->theme()->mWindowHeaderHeight));
	palette_scroller->setPosition( Vector2i(0, mWindow->theme()->mWindowHeaderHeight+1));
	mContent = new nanogui::Widget(palette_scroller);
	pfw->setContent(mContent);
	mContent->setFixedSize(Vector2i(palette_scroller->width()-20,  palette_scroller->height()));
	mLayout = new nanogui::AdvancedGridLayout({20, 0, 30, 0}, {});
	mLayout->setMargin(1);
	setFixedSize(Vector2i(150, 30));
	//mLayout->setColStretch(1, 0);
	mLayout->setColStretch(2, 1);
	mContent->setLayout(mLayout);
	mWindow->setPosition(pos);
	mWindow->setLayout( new nanogui::BoxLayout(nanogui::Orientation::Vertical) );
	mWindow->setVisible(true);
	return mWindow;
}

void PropertyFormHelper::setWindow(nanogui::Window *wind) {
	assert(mScreen);
	using namespace nanogui;
	mWindow = wind;
	mWindow->setSize(nanogui::Vector2i(320, 640));
	mWindow->setFixedSize(nanogui::Vector2i(320, 640));
	nanogui::VScrollPanel *palette_scroller = new nanogui::VScrollPanel(mWindow);
	palette_scroller->setSize(Vector2i(mWindow->width(), mWindow->height() - mWindow->theme()->mWindowHeaderHeight));
	palette_scroller->setFixedSize(Vector2i(mWindow->width(), mWindow->height() - mWindow->theme()->mWindowHeaderHeight));
	palette_scroller->setPosition( Vector2i(0, mWindow->theme()->mWindowHeaderHeight+1));
	mContent = new nanogui::Widget(palette_scroller);
	PropertyFormWindow *pfw = dynamic_cast<PropertyFormWindow*>(wind);
	if (pfw) pfw->setContent(mContent);
	mContent->setFixedSize(Vector2i(palette_scroller->width()-20,  palette_scroller->height()));
	mLayout = new nanogui::AdvancedGridLayout({20, 0, 30, 0}, {});
	mLayout->setMargin(1);
	setFixedSize(Vector2i(150, 30));
	//mLayout->setColStretch(1, 0);
	//mLayout->setColStretch(2, 1);
	mContent->setLayout(mLayout);
	mWindow->setLayout( new nanogui::BoxLayout(nanogui::Orientation::Vertical) );
	mWindow->setVisible(true);
}

nanogui::Widget *PropertyFormHelper::content() { return mContent; }

