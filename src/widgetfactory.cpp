#include "widgetfactory.h"

#include "editorlabel.h"
#include "editorimageview.h"
#include "editorprogressbar.h"
#include "editortextbox.h"
#include "editorlineplot.h"
#include "editorbutton.h"
#include "editorframe.h"
#include "editorcombobox.h"
#include <nanogui/button.h>
#include "helper.h"
#include "linkmanager.h"
#include "valuehelper.h"
#include "colourhelper.h"
#include "thememanager.h"
#include "editorlist.h"

static bool stringEndsWith(const std::string &src, const std::string ending) {
	size_t l_src = src.length();
	size_t l_end = ending.length();
	if (l_src < l_end) return false;
	return src.substr(src.end() - l_end - src.begin()) == ending;
}

void fixElementPosition(nanogui::Widget *w, const Value & vx, const Value & vy) {
	if (vx != SymbolTable::Null && vx != SymbolTable::Null) {
		long x, y;
		if (vx.asInteger(x) && vy.asInteger(y)) w->setPosition(nanogui::Vector2i(x,y));
	}
}

void fixElementPosition(nanogui::Widget *w, Structure *s) {
	fixElementPosition(w, s->getValue("pos_x"), s->getValue("pos_y"));
}

void fixElementSize(nanogui::Widget *w, const Value & vx, const Value & vy) {
	if (vx != SymbolTable::Null && vx != SymbolTable::Null) {
		long x, y;
		if (vx.asInteger(x) && vy.asInteger(y)) {
			w->setSize(nanogui::Vector2i(x,y));
			w->setFixedSize(nanogui::Vector2i(x,y));
		}
	}
}

void fixElementSize(nanogui::Widget *w, Structure *s) {
	fixElementSize(w, s->getValue("width"), s->getValue("height"));
}

void setElementPosition(const WidgetParams &params, nanogui::Widget *w, Structure *s) {
	fixElementPosition(w, s->getValue("pos_x"), s->getValue("pos_y"));
	if (params.offset != nanogui::Vector2i(0,0)) {
		auto pos = w->position();
		w->setPosition(pos - params.offset);
	}
}

WidgetParams::WidgetParams(Structure *structure, Widget *w, Structure *elem,
	EditorGUI *editor_gui, const nanogui::Vector2i &offset_) :
		s(structure), window(w), element(elem), gui(editor_gui),
		format_val(element->getValue("format")),
		connection(element->getValue("connection")),
		vis(element->getValue("visibility")),
		scale_val(element->getValue("value_scale")),
		border(element->getValue("border")),
		auto_update(element->getValue("auto_update")),
		working_text(element->getValue("working_text")),
		kind(element->getKind()), offset(offset_)
{
	StructureClass *element_class = findClass(kind);

	const Value font_size_val(element->getValue("font_size"));
	lp = nullptr;
	const Value remote_name(element->getValue("remote"));
	remote = remote_name == SymbolTable::Null || remote_name.asString().empty() || remote_name.asString() == "null"
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
		const Value wrap_v(element->getValue("wrap"));
		if (wrap_v != SymbolTable::Null) wrap_v.asBoolean(wrap);
	}

	ivis = false;
	{
		const Value ivis_v(element->getValue("inverted_visibility"));
		if (ivis_v != SymbolTable::Null) ivis_v.asBoolean(ivis);
	}
	font_size = 0;
	if (font_size_val != SymbolTable::Null) font_size_val.asInteger(font_size);

	const Value value_type_val(element->getValue("value_type"));
	value_type = -1;
	if (value_type_val != SymbolTable::Null) value_type_val.asInteger(value_type);
	value_scale = 1.0f;
	if (scale_val != SymbolTable::Null) scale_val.asFloat(value_scale);
	tab_pos = 0;
	const Value tab_pos_val(element->getValue("tab_pos"));
	if (tab_pos_val != SymbolTable::Null) tab_pos_val.asInteger(tab_pos);
	x_scale = 0;
	const Value x_scale_val(element->getValue("x_scale"));
	if (x_scale_val != SymbolTable::Null) x_scale_val.asFloat(x_scale);
	//const Value caption_v( (lp) ? lp->value() : (remote != SymbolTable::Null) ? "" : element->getValue("caption"));

	const Value theme_name(element->getValue("theme"));
	if (theme_name != SymbolTable::Null) {
		theme = ThemeManager::instance().findTheme(theme_name.asString());
	}
}

template <typename T>
void prepare_remote_links(const WidgetParams &params, T *w) {
	auto remote_links = LinkManager::instance().remote_links(params.s->getStructureDefinition()->getName(), w->getName());
	if (remote_links) {
		w->setRemoteLinks(remote_links);
		auto property_id_to_name = w->reverse_property_map();
		for (auto & link_info : *remote_links) {
			auto linkable_property = params.gui->findLinkableProperty(link_info.remote_name);
			if (linkable_property) {
				auto found_property = property_id_to_name.find(link_info.property_name);
				if (found_property != property_id_to_name.end()) {
					linkable_property->link(new LinkableObject(new PropertyLinkTarget(w, (*found_property).second, defaultForProperty(link_info.property_name))));
				}
			}
			else {
				//std::cout << "expecting to find linkable property for " << link_info.remote_name << " on " << w->getName() << "\n";
			}
		}
	}
}

void createLabel(WidgetParams &params) {
	const Value caption_v( (params.lp)
		? params.lp->value()
		: (params.remote != SymbolTable::Null)
			? ""
			: params.element->getValue("caption"));
	EditorLabel *el = new EditorLabel(params.s, params.window, params.element->getName(), params.lp,
		(caption_v != SymbolTable::Null) ? caption_v.asString() : "");
	el->setName(params.element->getName());
	el->setDefinition(params.element);
	if (params.theme.get()) { el->setTheme(params.theme); }
	setElementPosition(params, el, params.element);
	fixElementSize( el, params.element);
	if (params.connection != SymbolTable::Null) {
		el->setRemoteName(params.remote.asString());
		el->setConnection(params.connection.asString());
	}
	if (params.font_size) el->setFontSize(params.font_size);
	Value bg_colour(params.element->getValue("bg_color"));
	if (bg_colour != SymbolTable::Null)
		el->setBackgroundColor(colourFromProperty(params.element, "bg_color"));
	Value text_colour(params.element->getValue("text_colour"));
	if (text_colour != SymbolTable::Null)
		el->setTextColor(colourFromProperty(params.element, "text_colour"));
	Value alignment_v(params.element->getValue("alignment"));
	if (alignment_v != SymbolTable::Null) el->setPropertyValue("Alignment", alignment_v.asString());
	Value valignment_v(params.element->getValue("valign"));
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
	prepare_remote_links(params, el);
	el->setChanged(false);
}

void createList(WidgetParams &params) {
	EditorList *el = new EditorList(params.s, params.window, params.element->getName(), params.lp, "");
	el->setName(params.element->getName());
	el->setDefinition(params.element);
	if (params.theme.get()) { el->setTheme(params.theme); }
	setElementPosition(params, el, params.element);
	fixElementSize( el, params.element);
	if (params.connection != SymbolTable::Null) {
		el->setRemoteName(params.remote.asString());
		el->setConnection(params.connection.asString());
	}
	if (params.font_size) el->setFontSize(params.font_size);
	Value bg_colour(params.element->getValue("bg_color"));
	if (bg_colour != SymbolTable::Null)
		el->setBackgroundColor(colourFromProperty(params.element, "bg_color"));
	Value text_colour(params.element->getValue("text_colour"));
	if (text_colour != SymbolTable::Null)
		el->setTextColor(colourFromProperty(params.element, "text_colour"));
	Value alignment_v(params.element->getValue("alignment"));
	if (alignment_v != SymbolTable::Null) el->setPropertyValue("Alignment", alignment_v.asString());
	Value items_v(params.element->getValue("items"));
	if (items_v != SymbolTable::Null) {
		el->setItems(items_v.asString());
	}
	Value items_file_v(params.element->getValue("items_file"));
	if (items_file_v != SymbolTable::Null) {
		el->setItemFilename(items_file_v.asString());
	}
	Value selected_v(params.element->getValue("selected"));
	if (selected_v != SymbolTable::Null) {
		el->setSelected(selected_v.asString());
	}
	Value selind_v(params.element->getValue("selected_index"));
	if (selind_v != SymbolTable::Null) {
		long idx;
		if (selind_v.asInteger(idx)) {
			el->select(idx);
		}
	}
	Value valignment_v(params.element->getValue("valign"));
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
	prepare_remote_links(params, el);
	el->setChanged(false);
}

void createImage(WidgetParams &params) {
	EditorImageView *el = new EditorImageView(params.s, params.window, params.element->getName(), params.lp, 0);
	el->setName(params.element->getName());
	el->setDefinition(params.element);
	if (params.connection != SymbolTable::Null) {
		el->setConnection(params.connection.asString());
	}
	if (params.theme.get()) { el->setTheme(params.theme); }
	const Value img_scale_val(params.element->getValue("scale"));
	double img_scale = 1.0f;
	if (img_scale_val != SymbolTable::Null) img_scale_val.asFloat(img_scale);
	setElementPosition(params, el, params.element);
	fixElementSize( el, params.element);
	if (params.font_size) el->setFontSize(params.font_size);
	if (params.format_val != SymbolTable::Null) el->setValueFormat(params.format_val.asString());
	if (params.value_type != -1) el->setValueType(params.value_type);
	if (params.value_scale != 1.0) el->setValueScale( params.value_scale );
	el->setScale( img_scale );
	if (params.tab_pos) el->setTabPosition(params.tab_pos);
	el->setInvertedVisibility(params.ivis);
	const Value image_file_v( (params.lp) ? params.lp->value() : (params.element->getValue("image_file")));
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
	prepare_remote_links(params, el);
	el->setChanged(false);
}

void createProgress(WidgetParams &params) {
	EditorProgressBar *ep = new EditorProgressBar(params.s, params.window, params.element->getName(), params.lp);
	ep->setDefinition(params.element);
	if (params.theme.get()) { ep->setTheme(params.theme); }
	setElementPosition(params, ep, params.element);
	fixElementSize( ep, params.element);
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
	{
	Value bg_colour(params.element->getValue("bg_color"));
	if (bg_colour != SymbolTable::Null)
		ep->setBackgroundColor(colourFromProperty(params.element, "bg_color"));
	}
	{
	Value fg_colour(params.element->getValue("fg_color"));
	if (fg_colour != SymbolTable::Null)
		ep->setColor(colourFromProperty(params.element, "fg_color"));
	}
	prepare_remote_links(params, ep);
	ep->setChanged(false);
}

void createText(WidgetParams &params) {
	EditorTextBox *textBox = new EditorTextBox(params.s, params.window, params.element->getName(), params.lp);
	textBox->setDefinition(params.element);
	if (params.theme.get()) { textBox->setTheme(params.theme); }
	const Value text_v( (params.lp) ? params.lp->value() : (params.remote != SymbolTable::Null) ? "" : params.element->getValue("text"));
	if (text_v != SymbolTable::Null) textBox->setValue(text_v.asString());
	const Value alignment_v(params.element->getValue("alignment"));
	if (alignment_v != SymbolTable::Null) textBox->setPropertyValue("Alignment", alignment_v.asString());
	const Value valignment_v(params.element->getValue("valign"));
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
	setElementPosition(params, textBox, params.element);
	fixElementSize( textBox, params.element);
	if (params.font_size) textBox->setFontSize(params.font_size);
	if (params.tab_pos) textBox->setTabPosition(params.tab_pos);
	if (params.border != SymbolTable::Null) textBox->setBorder(params.border.iValue);
	if (params.auto_update != SymbolTable::Null) {
		bool aa;
		if (params.auto_update.asBoolean(aa)) {
			textBox->auto_update = aa;
		}
	}
	if (params.working_text != SymbolTable::Null) {
		textBox->working_text = params.working_text.asString();
	}
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
		{
			std::string msg = value;
			int size = textBox->getRemote()->dataSize();
			if (size < msg.length()) {
				msg = msg.substr(size);
			}
			params.gui->queueMessage(conn,
				params.gui->getIODSyncCommand(conn, textBox->getRemote()->address_group(),
					textBox->getRemote()->address(), msg.c_str()), 
						[](std::string s){ extern int debug; if (debug) { std::cout << s << "\n"; } });
			extern int debug;
			if (debug) { std::cout << "sending: " << value << " of size " << size << "\n"; }
		}
		return false;
	});
	prepare_remote_links(params, textBox);
}

void createPlot(WidgetParams &params) {
	EditorLinePlot *lp = new EditorLinePlot(params.s, params.window, params.element->getName(), nullptr);
	lp->setDefinition(params.element);
	if (params.theme.get()) { lp->setTheme(params.theme); }
	lp->setBufferSize(params.gui->sampleBufferSize());
	setElementPosition(params, lp, params.element);
	fixElementSize( lp, params.element);
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
	if (params.element->getValue("overlay_plots").asBoolean(should_overlay_plots))
		lp->overlay(should_overlay_plots);
	}
	const Value monitors(params.element->getValue("monitors"));
	lp->setInvertedVisibility(params.ivis);
	if (monitors != SymbolTable::Null) {
		lp->setMonitors(params.gui->getUserWindow(), monitors.asString());
	}
	lp->setChanged(false);
	if (params.visibility) lp->setVisibilityLink(params.visibility);
	prepare_remote_links(params, lp);
}

void createButton(WidgetParams &params) {
	const Value caption_v(params.element->getValue("caption"));
	EditorButton *b = new EditorButton(params.s, params.window, params.element->getName(), params.lp,
									   (caption_v != SymbolTable::Null)?caption_v.asString(): "");
	if (params.kind == "INDICATOR") b->setEnabled(false); else b->setEnabled(true);
	b->setDefinition(params.element);
	if (params.theme.get()) { b->setTheme(params.theme); }
	b->setBackgroundColor(colourFromProperty(params.element, "bg_color"));
	b->setTextColor(colourFromProperty(params.element, "text_colour"));
	b->setOnColor(colourFromProperty(params.element, "bg_on_color"));
	b->setOnTextColor(colourFromProperty(params.element, "text_on_colour"));
	long enum_val;
	if (params.element->getValue("border_style").asInteger(enum_val)) {
		b->setBorderStyle(static_cast<EditorButton::BorderStyle>(enum_val));
	}
	if (params.element->getValue("border_grad_dir").asInteger(enum_val)) {
		b->setBorderGradientDir(static_cast<EditorButton::BorderGradientDirection>(enum_val));
	}
	if (params.element->getValue("border_colouring").asInteger(enum_val)) {
		b->setBorderColouring(static_cast<EditorButton::BorderColouring>(enum_val));
	}
	b->setBorderGradTop(colourFromProperty(params.element, "border_grad_top"));
	b->setBorderGradBot(colourFromProperty(params.element, "border_grad_bot"));

	b->setFlags(params.element->getIntProperty("behaviour", nanogui::Button::NormalButton));
	if (params.connection != SymbolTable::Null) {
		b->setRemoteName(params.remote.asString());
		b->setConnection(params.connection.asString());
	}
	setElementPosition(params, b, params.element);
	fixElementSize( b, params.element);
	if (params.font_size) b->setFontSize(params.font_size);
	if (params.tab_pos) b->setTabPosition(params.tab_pos);
	if (params.border != SymbolTable::Null) b->setBorder(params.border.iValue);
	b->setInvertedVisibility(params.ivis);
	b->setWrap(params.wrap);
	const Value alignment_v(params.element->getValue("alignment"));
	if (alignment_v != SymbolTable::Null) b->setPropertyValue("Alignment", alignment_v);
	const Value valignment_v(params.element->getValue("valign"));
	if (valignment_v != SymbolTable::Null) b->setPropertyValue("Vertical Alignment", valignment_v);
	if (params.format_val != SymbolTable::Null) b->setValueFormat(params.format_val.asString());
	if (params.value_type != -1) b->setValueType(params.value_type);
	if (params.value_scale != 1.0) b->setValueScale( params.value_scale );
	{
		const Value caption_v = params.element->getValue("caption");
		if (caption_v != SymbolTable::Null) b->setCaption(caption_v.asString());
	}
	{
		const Value caption_v = params.element->getValue("on_caption");
		if (caption_v != SymbolTable::Null) {
			b->setOnCaption(caption_v.asString());
		}
	}
	{
		const Value cmd(params.element->getValue("command"));
		if (cmd != SymbolTable::Null && cmd.asString().length()) b->setCommand(cmd.asString());
	}
	b->setupButtonCallbacks(params.lp, params.gui);
	b->setImageName(params.element->getValue("image").asString());
	if (params.visibility) b->setVisibilityLink(params.visibility);
	prepare_remote_links(params, b);
	b->setChanged(false);
}


void createFrame(WidgetParams &params) {
	auto ep = new EditorFrame(params.s, params.window, params.element->getName(), params.lp);
	ep->setDefinition(params.element);
	setElementPosition(params, ep, params.element);
	fixElementSize( ep, params.element);
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

void createComboBox(WidgetParams &params) {
	EditorComboBox *el = new EditorComboBox(params.s, params.window, params.element->getName(), params.lp);
	el->setName(params.element->getName());
	el->setDefinition(params.element);
	if (params.theme.get()) { el->setTheme(params.theme); }
	setElementPosition(params, el, params.element);
	fixElementSize( el, params.element);
	if (params.connection != SymbolTable::Null) {
		el->setRemoteName(params.remote.asString());
		el->setConnection(params.connection.asString());
	}
	if (params.font_size) el->setFontSize(params.font_size);
	Value bg_colour(params.element->getValue("bg_color"));
	if (bg_colour != SymbolTable::Null)
		el->setBackgroundColor(colourFromProperty(params.element, "bg_color"));
	Value text_colour(params.element->getValue("text_colour"));
	if (text_colour != SymbolTable::Null)
		el->setTextColor(colourFromProperty(params.element, "text_colour"));
	if (params.format_val != SymbolTable::Null) el->setValueFormat(params.format_val.asString());
	if (params.value_type != -1) el->setValueType(params.value_type);
	if (params.value_scale != 1.0) el->setValueScale( params.value_scale );
	if (params.tab_pos) el->setTabPosition(params.tab_pos);
	if (params.border != SymbolTable::Null) el->setBorder(params.border.iValue);
	el->setInvertedVisibility(params.ivis);
	if (params.visibility) el->setVisibilityLink(params.visibility);
	prepare_remote_links(params, el);
	el->setChanged(false);
}



