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

namespace nanogui {
using Vector2d = Eigen::Vector2d;
using VectorXd = Eigen::VectorXd;
using MatrixXd = Eigen::MatrixXd;
using Matrix3d = Eigen::Matrix3d;
}

using nanogui::Vector2d;
using nanogui::MatrixXd;
using nanogui::Matrix3d;

class PropertyFormHelper;

class EditorWidget : public Selectable, public EditorObject, public Connectable {

public:

	EditorWidget(NamedObject *owner, const std::string structure_name, nanogui::Widget *w, LinkableProperty *lp);
	EditorWidget(NamedObject *owner, const std::string structure_name, const std::string &nam,
				 nanogui::Widget *w, LinkableProperty *lp);
	~EditorWidget();

	static EditorWidget *create(const std::string kind);

	virtual nanogui::Widget *asWidget() { return nullptr; }

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
	virtual void drawElementBorder(NVGcontext *ctx, nanogui::Vector2i pos, nanogui::Vector2i size);

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
	virtual const std::map<std::string, std::string> & property_map() const;
	virtual const std::map<std::string, std::string> & reverse_property_map() const;

	float valueScale();
	void setValueScale(float s);

	int tabPosition();
	void setTabPosition(int p);

	void addLink(const Link &new_link);
	void addLink(Link *new_link);
	void removeLink(Anchor *src, Anchor *dest);
	void updateLinks();

	void setVisibilityLink(LinkableProperty *lp);
	void setInvertedVisibility(bool which) { inverted_visibility = which; }
	bool invertedVisibility() { return inverted_visibility; }

	void setRemoteName(const std::string rn) { remote_name = rn; }
	std::string getRemoteName() { return remote_name; }
	void setConnection(const std::string c) { connection_name = c; }
	std::string getConnection() { return connection_name; }

	void setBorder(int val) { border = val; }
	int getBorder() { return border; }

	const std::string &getValueFormat();
	void setValueFormat(const std::string fmt);
	int getValueType();
	void setValueType(int val);

protected:
	std::list<Link>links;
	std::string base;
	std::string remote_name; // only used if there is no remote
	std::string connection_name; // only used if there is no remote
	nanogui::DragHandle *dh;
	std::vector<Handle> handles;
	MatrixXd handle_coordinates;
	std::string pattern_list;
	Structure *definition;
	std::string format_string;
	float value_scale;
	int tab_position;
	LinkableProperty *visibility;
	bool inverted_visibility;
	int border;
	int value_type;
};

#endif
