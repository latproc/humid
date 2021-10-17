#include "widgetfactory.h"

#include "editorlabel.h"
#include "editorimageview.h"
#include "editorprogressbar.h"
#include "editortextbox.h"
#include "editorlineplot.h"
#include "editorbutton.h"
#include "editorframe.h"
#include <nanogui/button.h>
#include "helper.h"
#include "linkmanager.h"
#include "valuehelper.h"
#include "colourhelper.h"

static bool stringEndsWith(const std::string &src, const std::string ending) {
	size_t l_src = src.length();
	size_t l_end = ending.length();
	if (l_src < l_end) return false;
	return src.substr(src.end() - l_end - src.begin()) == ending;
}

void fixElementPosition(nanogui::Widget *w, const SymbolTable &properties);
void fixElementSize(nanogui::Widget *w, const SymbolTable &properties);

void setElementPosition(const WidgetParams &params, nanogui::Widget *w, const SymbolTable &properties) {
	fixElementPosition(w, properties);
	if (params.offset != nanogui::Vector2i(0,0)) {
		auto pos = w->position();
		w->setPosition(pos - params.offset);
	}
}

WidgetParams::WidgetParams(Structure *structure, Widget *w, Structure *elem,
	EditorGUI *editor_gui, const nanogui::Vector2i &offset_) :
		s(structure), window(w), element(elem), gui(editor_gui),
		format_val(element->getProperties().find("format")),
		connection(element->getProperties().find("connection")),
		vis(element->getProperties().find("visibility")),
		scale_val(element->getProperties().find("value_scale")),
		border(element->getProperties().find("border")),
		kind(element->getKind()), offset(offset_)
{
	StructureClass *element_class = findClass(kind);
	
	const Value font_size_val(element->getProperties().find("font_size"));
	lp = nullptr;
	const Value remote_name(element->getProperties().find("remote"));
	remote = remote_name == SymbolTable::Null || remote_name.asString() == "null"
		? SymbolTable::Null
		: remote_name;
	if (remote != SymbolTable::Null) {
		std::string lp_name = remote.asString();
		if (lp_name != "null") {
			lp = gui->findLinkableProperty(lp_name);
			if (!lp) {
				if (stringEndsWith(lp_name, ".VALUE")) {
					lp_name = lp_name.substr(0, lp_name.length()-6);
					lp = gui->findLinkableProperty(lp_name);
				}
			}
		}
	}
	visibility = nullptr;
	if (vis != SymbolTable::Null)
		visibility = gui->findLinkableProperty(vis.asString());

	wrap = false;
	{
		const Value wrap_v(element->getProperties().find("wrap"));
		if (wrap_v != SymbolTable::Null) wrap_v.asBoolean(wrap);
	}

	ivis = false;
	{
		const Value ivis_v(element->getProperties().find("inverted_visibility"));
		if (ivis_v != SymbolTable::Null) ivis_v.asBoolean(ivis);
	}
	font_size = 0;
	if (font_size_val != SymbolTable::Null) font_size_val.asInteger(font_size);

	const Value value_type_val(element->getProperties().find("value_type"));
	value_type = -1;
	if (value_type_val != SymbolTable::Null) value_type_val.asInteger(value_type);
	value_scale = 1.0f;
	if (scale_val != SymbolTable::Null) scale_val.asFloat(value_scale);
	tab_pos = 0;
	const Value tab_pos_val(element->getProperties().find("tab_pos"));
	if (tab_pos_val != SymbolTable::Null) tab_pos_val.asInteger(tab_pos);
	x_scale = 0;
	const Value x_scale_val(element->getProperties().find("x_scale"));
	if (x_scale_val != SymbolTable::Null) x_scale_val.asFloat(x_scale);
	//const Value caption_v( (lp) ? lp->value() : (remote != SymbolTable::Null) ? "" : element->getProperties().find("caption"));

	const Value theme_name(element->getProperties().find("theme"));
	if (theme_name != SymbolTable::Null) {
	}
}

void createLabel(WidgetParams &params) {
	const Value caption_v( (params.lp)
		? params.lp->value()
		: (params.remote != SymbolTable::Null)
			? ""
			: params.element->getProperties().find("caption"));
	EditorLabel *el = new EditorLabel(params.s, params.window, params.element->getName(), params.lp,
		(caption_v != SymbolTable::Null) ? caption_v.asString() : "");
	el->setName(params.element->getName());
	el->setDefinition(params.element);
	setElementPosition(params, el, params.element->getProperties());
	fixElementSize( el, params.element->getProperties());
	if (params.connection != SymbolTable::Null) {
		el->setRemoteName(params.remote.asString());
		el->setConnection(params.connection.asString());
	}
	if (params.font_size) el->setFontSize(params.font_size);
	Value bg_colour(params.element->getProperties().find("bg_color"));
	if (bg_colour != SymbolTable::Null)
		el->setBackgroundColor(colourFromProperty(params.element, "bg_color"));
	Value text_colour(params.element->getProperties().find("text_colour"));
	if (text_colour != SymbolTable::Null)
		el->setTextColor(colourFromProperty(params.element, "text_colour"));
	Value alignment_v(params.element->getProperties().find("alignment"));
	if (alignment_v != SymbolTable::Null) el->setPropertyValue("Alignment", alignment_v.asString());
	Value valignment_v(params.element->getProperties().find("valign"));
	if (valignment_v != SymbolTable::Null) el->setPropertyValue("Vertical Alignment", valignment_v.asString());
	if (params.format_val != SymbolTable::Null) el->setValueFormat(params.format_val.asString());
	if (params.value_type != -1) el->setValueType(params.value_type);				
	if (params.value_scale != 1.0) el->setValueScale( params.value_scale );
	if (params.tab_pos) el->setTabPosition(params.tab_pos);
	if (params.lp)
		params.lp->link(new LinkableText(el));
	if (params.border != SymbolTable::Null) el->setBorder(params.border.iValue);
	el->setInvertedVisibility(params.ivis);
	if (params.visibility) el->setVisibilityLink(params.visibility);

	auto remote_links = LinkManager::instance().remote_links(params.s->getStructureDefinition()->getName(), el->getName());
	if (remote_links) {
		auto property_id_to_name = el->reverse_property_map();
		for (auto & link_info : *remote_links) {
			auto linkable_property = params.gui->findLinkableProperty(link_info.remote_name);
			if (linkable_property) {
				auto found_property = property_id_to_name.find(link_info.property_name);
				if (found_property != property_id_to_name.end()) {
					linkable_property->link(new LinkableObject(new PropertyLinkTarget(el, (*found_property).second, defaultForProperty(link_info.property_name))));
				}
			}
			else {
				std::cout << "expecting to find linkable property for " << link_info.remote_name << " on " << el->getName() << "\n";
			}
		}
	}

	el->setChanged(false);
}

void createImage(WidgetParams &params) {
	EditorImageView *el = new EditorImageView(params.s, params.window, params.element->getName(), params.lp, 0);
	el->setName(params.element->getName());
	el->setDefinition(params.element);
	if (params.connection != SymbolTable::Null) {
		el->setConnection(params.connection.asString());
	}
	const Value img_scale_val(params.element->getProperties().find("scale"));
	double img_scale = 1.0f;
	if (img_scale_val != SymbolTable::Null) img_scale_val.asFloat(img_scale);
	setElementPosition(params, el, params.element->getProperties());
	fixElementSize( el, params.element->getProperties());
	if (params.font_size) el->setFontSize(params.font_size);
	if (params.format_val != SymbolTable::Null) el->setValueFormat(params.format_val.asString());
	if (params.value_type != -1) el->setValueType(params.value_type);				
	if (params.value_scale != 1.0) el->setValueScale( params.value_scale );
	el->setScale( img_scale );
	if (params.tab_pos) el->setTabPosition(params.tab_pos);
	el->setInvertedVisibility(params.ivis);
	const Value image_file_v( (params.lp) ? params.lp->value() : (params.element->getProperties().find("image_file")));
	if (image_file_v != SymbolTable::Null) {
		std::string ifn = image_file_v.asString();
		el->setImageName(ifn);
		el->fit();
	}
	if (params.border != SymbolTable::Null) el->setBorder(params.border.iValue);
	if (params.lp) {
		params.lp->link(new LinkableText(el));
	if (params.visibility) el->setVisibilityLink(params.visibility);
	}
	
	auto remote_links = LinkManager::instance().remote_links(params.s->getStructureDefinition()->getName(), el->getName());
	if (remote_links) {
		auto property_id_to_name = el->reverse_property_map();
		for (auto & link_info : *remote_links) {
			auto linkable_property = params.gui->findLinkableProperty(link_info.remote_name);
			if (linkable_property) {
				auto found_property = property_id_to_name.find(link_info.property_name);
				if (found_property != property_id_to_name.end()) {
					linkable_property->link(new LinkableObject(new PropertyLinkTarget(el, (*found_property).second, defaultForProperty(link_info.property_name))));
				}
			}
			else {
				std::cout << "expecting to find linkable property for " << link_info.remote_name << " on " << el->getName() << "\n";
			}
		}
	}
	el->setChanged(false);
}

void createProgress(WidgetParams &params) {
	EditorProgressBar *ep = new EditorProgressBar(params.s, params.window, params.element->getName(), params.lp);
	ep->setDefinition(params.element);
	setElementPosition(params, ep, params.element->getProperties());
	fixElementSize( ep, params.element->getProperties());
	if (params.connection != SymbolTable::Null) {
		ep->setRemoteName(params.remote.asString());
		ep->setConnection(params.connection.asString());
	}
	if (params.format_val != SymbolTable::Null) ep->setValueFormat(params.format_val.asString());
	if (params.value_type != -1) ep->setValueType(params.value_type);				
	if (params.value_scale != 1.0) ep->setValueScale( params.value_scale );
	if (params.tab_pos) ep->setTabPosition(params.tab_pos);
	if (params.border != SymbolTable::Null) ep->setBorder(params.border.iValue);
	ep->setInvertedVisibility(params.ivis);
	ep->setChanged(false);
	if (params.lp)
		params.lp->link(new LinkableNumber(ep));
	if (params.visibility) ep->setVisibilityLink(params.visibility);
}

void createText(WidgetParams &params) {
	EditorTextBox *textBox = new EditorTextBox(params.s, params.window, params.element->getName(), params.lp);
	textBox->setDefinition(params.element);
	const Value text_v( (params.lp) ? params.lp->value() : (params.remote != SymbolTable::Null) ? "" : params.element->getProperties().find("text"));
	if (text_v != SymbolTable::Null) textBox->setValue(text_v.asString());
	const Value alignment_v(params.element->getProperties().find("alignment"));
	if (alignment_v != SymbolTable::Null) textBox->setPropertyValue("Alignment", alignment_v.asString());
	const Value valignment_v(params.element->getProperties().find("valign"));
	if (valignment_v != SymbolTable::Null) textBox->setPropertyValue("Vertical Alignment", valignment_v.asString());
	textBox->setEnabled(true);
	textBox->setEditable(true);
	if (params.connection != SymbolTable::Null) {
		textBox->setRemoteName(params.remote.asString());
		textBox->setConnection(params.connection.asString());
	}
	if (params.format_val != SymbolTable::Null) textBox->setValueFormat(params.format_val.asString());
	if (params.value_type != -1) textBox->setValueType(params.value_type);				
	if (params.value_scale != 1.0) textBox->setValueScale( params.value_scale );
	setElementPosition(params, textBox, params.element->getProperties());
	fixElementSize( textBox, params.element->getProperties());
	if (params.font_size) textBox->setFontSize(params.font_size);
	if (params.tab_pos) textBox->setTabPosition(params.tab_pos);
	if (params.border != SymbolTable::Null) textBox->setBorder(params.border.iValue);
	textBox->setName(params.element->getName());
	if (params.lp)
		textBox->setTooltip(params.remote.asString());
	else
		textBox->setTooltip(params.element->getName());
	if (params.lp)
		params.lp->link(new LinkableText(textBox));
	if (params.visibility) textBox->setVisibilityLink(params.visibility);
	textBox->setInvertedVisibility(params.ivis);
	textBox->setChanged(false);
	textBox->setCallback( [textBox, params](const std::string &value)->bool{
		if (!textBox->getRemote()) return true;
		const std::string &conn = textBox->getRemote()->group();
		char *rest = 0;
		{
			long val = strtol(value.c_str(),&rest,10);
			if (*rest == 0) {
				params.gui->queueMessage(conn,
						params.gui->getIODSyncCommand(conn,
						textBox->getRemote()->address_group(),
						textBox->getRemote()->address(), (int)(val * textBox->valueScale()) ),
					[](std::string s){std::cout << s << "\n"; });
				return true;
			}
		}
		{
			double val = strtod(value.c_str(),&rest);
			if (*rest == 0)  {
				params.gui->queueMessage(conn,
					params.gui->getIODSyncCommand(conn, textBox->getRemote()->address_group(),
						textBox->getRemote()->address(), (float)(val * textBox->valueScale())) , [](std::string s){std::cout << s << "\n"; });
				return true;
			}
		}
		return false;
	});
	
	auto remote_links = LinkManager::instance().remote_links(params.s->getStructureDefinition()->getName(), textBox->getName());
	if (remote_links) {
		auto property_id_to_name = textBox->reverse_property_map();
		for (auto & link_info : *remote_links) {
			auto linkable_property = params.gui->findLinkableProperty(link_info.remote_name);
			if (linkable_property) {
				auto found_property = property_id_to_name.find(link_info.property_name);
				if (found_property != property_id_to_name.end()) {
					linkable_property->link(new LinkableObject(new PropertyLinkTarget(textBox, (*found_property).second, defaultForProperty(link_info.property_name))));
				}
			}
			else {
				std::cout << "expecting to find linkable property for " << link_info.remote_name << " on " << textBox->getName() << "\n";
			}
		}
	}
}

void createPlot(WidgetParams &params) {
	EditorLinePlot *lp = new EditorLinePlot(params.s, params.window, params.element->getName(), nullptr);
	lp->setDefinition(params.element);
	lp->setBufferSize(params.gui->sampleBufferSize());
	setElementPosition(params, lp, params.element->getProperties());
	fixElementSize( lp, params.element->getProperties());
	if (params.connection != SymbolTable::Null) {
		lp->setRemoteName(params.remote.asString());
		lp->setConnection(params.connection.asString());
	}
	if (params.format_val != SymbolTable::Null) lp->setValueFormat(params.format_val.asString());
	if (params.value_type != -1) lp->setValueType(params.value_type);				
	if (params.value_scale != 1.0) lp->setValueScale( params.value_scale );
	if (params.font_size) lp->setFontSize(params.font_size);
	if (params.tab_pos) lp->setTabPosition(params.tab_pos);
	if (params.x_scale) lp->setTimeScale(params.x_scale);
	{
	bool should_overlay_plots;
	if (params.element->getProperties().find("overlay_plots").asBoolean(should_overlay_plots))
		lp->overlay(should_overlay_plots);
	}
	const Value monitors(params.element->getProperties().find("monitors"));
	lp->setInvertedVisibility(params.ivis);
	if (monitors != SymbolTable::Null) {
		lp->setMonitors(params.gui->getUserWindow(), monitors.asString());
	}
	lp->setChanged(false);
	if (params.visibility) lp->setVisibilityLink(params.visibility);
}

void createButton(WidgetParams &params) {
	const Value caption_v(params.element->getProperties().find("caption"));
	EditorButton *b = new EditorButton(params.s, params.window, params.element->getName(), params.lp,
									   (caption_v != SymbolTable::Null)?caption_v.asString(): "");
	if (params.kind == "INDICATOR") b->setEnabled(false); else b->setEnabled(true);
	b->setDefinition(params.element);
	b->setBackgroundColor(colourFromProperty(params.element, "bg_color"));
	b->setTextColor(colourFromProperty(params.element, "text_colour"));
	b->setOnColor(colourFromProperty(params.element, "bg_on_color"));
	b->setOnTextColor(colourFromProperty(params.element, "text_on_colour"));
	b->setFlags(params.element->getIntProperty("behaviour", nanogui::Button::NormalButton));
	if (params.connection != SymbolTable::Null) {
		b->setRemoteName(params.remote.asString());
		b->setConnection(params.connection.asString());
	}
	setElementPosition(params, b, params.element->getProperties());
	fixElementSize( b, params.element->getProperties());
	if (params.font_size) b->setFontSize(params.font_size);
	if (params.tab_pos) b->setTabPosition(params.tab_pos);
	if (params.border != SymbolTable::Null) b->setBorder(params.border.iValue);
	b->setInvertedVisibility(params.ivis);
	b->setWrap(params.wrap);
	const Value alignment_v(params.element->getProperties().find("alignment"));
	if (alignment_v != SymbolTable::Null) b->setPropertyValue("Alignment", alignment_v);
	const Value valignment_v(params.element->getProperties().find("valign"));
	if (valignment_v != SymbolTable::Null) b->setPropertyValue("Vertical Alignment", valignment_v);

	{
		const Value caption_v = params.element->getProperties().find("caption");
		if (caption_v != SymbolTable::Null) b->setCaption(caption_v.asString());
	}
	{
		const Value caption_v = params.element->getProperties().find("on_caption");
		if (caption_v != SymbolTable::Null) b->setOnCaption(caption_v.asString());
	}
	{
		const Value cmd(params.element->getProperties().find("command"));
		if (cmd != SymbolTable::Null && cmd.asString().length()) b->setCommand(cmd.asString());
	}
	b->setupButtonCallbacks(params.lp, params.gui);
	b->setImageName(params.element->getProperties().find("image").asString());
	if (params.visibility) b->setVisibilityLink(params.visibility);
	
	auto remote_links = LinkManager::instance().remote_links(params.s->getStructureDefinition()->getName(), b->getName());
	if (remote_links) {
		auto property_id_to_name = b->reverse_property_map();
		for (auto & link_info : *remote_links) {
			auto linkable_property = params.gui->findLinkableProperty(link_info.remote_name);
			if (linkable_property) {
				auto found_property = property_id_to_name.find(link_info.property_name);
				if (found_property != property_id_to_name.end()) {
					linkable_property->link(new LinkableObject(new PropertyLinkTarget(b, (*found_property).second, defaultForProperty(link_info.property_name))));
				}
			}
			else {
				std::cout << "expecting to find linkable property for " << link_info.remote_name << " on " << b->getName() << "\n";
			}
		}
	}

	b->setChanged(false);
}


void createFrame(WidgetParams &params) {
	auto ep = new EditorFrame(params.s, params.window, params.element->getName(), params.lp);
	ep->setDefinition(params.element);
	setElementPosition(params, ep, params.element->getProperties());
	fixElementSize( ep, params.element->getProperties());
	if (params.connection != SymbolTable::Null) {
		ep->setRemoteName(params.remote.asString());
		ep->setConnection(params.connection.asString());
	}
	if (params.format_val != SymbolTable::Null) ep->setValueFormat(params.format_val.asString());
	if (params.value_type != -1) ep->setValueType(params.value_type);
	if (params.value_scale != 1.0) ep->setValueScale( params.value_scale );
	if (params.tab_pos) ep->setTabPosition(params.tab_pos);
	if (params.border != SymbolTable::Null) ep->setBorder(params.border.iValue);
	ep->setInvertedVisibility(params.ivis);
	ep->setChanged(false);
	if (params.lp)
		params.lp->link(new LinkableNumber(ep));
	if (params.visibility) ep->setVisibilityLink(params.visibility);
}



