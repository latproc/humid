//
//  EditorWidget.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __EditorWidget_h__
#define __EditorWidget_h__

#include <ostream>
#include <string>
#include <map>
#include <vector>

#include <nanogui/common.h>
#include <nanogui/widget.h>

#include "selectable.h"
#include "editorobject.h"
#include "connectable.h"
#include "linkableobject.h"
#include "propertymonitor.h"
#include "structure.h"
#include "draghandle.h"
#include "linkableproperty.h"
#include "anchor.h"

using Eigen::Vector2d;
using Eigen::MatrixXd;
using Eigen::Matrix3d;

class PropertyFormHelper;

class EditorWidget : public Selectable, public EditorObject, public Connectable {

public:

	EditorWidget(const std::string structure_name, nanogui::Widget *w, LinkableProperty *lp);
	EditorWidget(const std::string structure_name, const std::string &nam,
				 nanogui::Widget *w, LinkableProperty *lp);
	~EditorWidget();

	static EditorWidget *create(const std::string kind);

	//nanogui::Widget *getWidget() { return widget; }

	virtual void getPropertyNames(std::list<std::string> &names);
	virtual void loadProperties(PropertyFormHelper *pfh);
	virtual void setProperty(const std::string &prop, const std::string value);
	virtual std::string getProperty(const std::string &prop);
	virtual Value getPropertyValue(const std::string &prop);
	virtual void setPropertyValue(const std::string &prop, const Value &v);

	virtual bool editorMouseButtonEvent(nanogui::Widget *widget, const nanogui::Vector2i &p, int button, bool down, int modifiers);

	virtual bool editorMouseEnterEvent(nanogui::Widget *widget, const nanogui::Vector2i &p, bool enter);

	bool editorMouseMotionEvent(nanogui::Widget *widget, const nanogui::Vector2i &p,
        const nanogui::Vector2i &rel, int button, int modifiers);

	void updateHandles(nanogui::Widget *w);

	virtual void drawSelectionBorder(NVGcontext *ctx, nanogui::Vector2i pos, nanogui::Vector2i size);

	std::string baseName() const;

	const std::string &getName() const;

	void justSelected() override;
	void justDeselected() override;


	void setPatterns(const std::string patterns);
	const std::string &patterns() const;

	void setDefinition(Structure *defn);
	Structure *getDefinition();
	virtual void updateStructure(); // update the structure properties to reflect the object
	virtual void loadPropertyToStructureMap(std::map<std::string, std::string> &property_map);

	float valueScale();
	void setValueScale(float s);

	int tabPosition();
	void setTabPosition(int p);

	void addLink(const Link &new_link);
	void addLink(Link *new_link);
	void removeLink(Anchor *src, Anchor *dest);
	void updateLinks();

protected:
	std::list<Link>links;
	std::string base;
	nanogui::DragHandle *dh;
	std::vector<Handle> handles;
	MatrixXd handle_coordinates;
	std::string pattern_list;
	Structure *definition;
	float value_scale;
	int tab_position;
};

#endif
