#ifndef __EditorTable_h__
#define __EditorTable_h__

#include <nanogui/vscrollpanel.h>
#include <nanogui/layout.h>
#include <cJSON.h>
#define cJSON_IsArray(item)   ((item) && ((item)->type == cJSON_Array))
#define cJSON_IsString(item)  ((item) && ((item)->type == cJSON_String))
#define cJSON_IsObject(item) ((item) && ((item)->type == cJSON_Object))
#define cJSON_IsNumber(item) ((item) && ((item)->type == cJSON_Number))
#include "editorlabel.h"

class EditorTable : public nanogui::Widget, public EditorWidget {
public:
    EditorTable(NamedObject *owner,
                nanogui::Widget *parent,
                const std::string nam,
                LinkableProperty *lp,
                cJSON *data);
    ~EditorTable();

    nanogui::Widget *asWidget() override { return this; }
    bool mouseButtonEvent(const nanogui::Vector2i &top_left, int button, bool down, int modifiers) override;

    bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;

    bool mouseEnterEvent(const Vector2i &p, bool enter) override;

    void setData(cJSON *data);
    cJSON *data() const { return mData; }

    void setHeader(cJSON *header);

    void setSelectedRow(int index);

    EditorLabel *find_or_create_label(const std::string & name, const std::string & text);

    int selectedRow() const { return mSelectedRow; }
    void clearSelection();

    // EditorWidget overrides
    void getPropertyNames(std::list<std::string> &names) override;
    void loadProperties(PropertyFormHelper* properties) override;
    void loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) override;
    const std::map<std::string, std::string> & property_map() const override;
    const std::map<std::string, std::string> & reverse_property_map() const override;
    Value getPropertyValue(const std::string &prop) override;
    void setProperty(const std::string &prop, const std::string value) override;
    void draw(NVGcontext *ctx) override;

protected:
    void rebuild();
    void rebuildHeader();

    int alignment = 1;
    int valign = 1;
    cJSON *mData = nullptr;
    cJSON *mHeaderSpec = nullptr;
    nanogui::VScrollPanel *mScroll;
    nanogui::Widget *mContainer;
    std::vector<EditorLabel*> mRows;
    int mSelectedRow = -1;
    LinkableProperty *mLinkedOption;
    EditorLabel *header = nullptr;
};

#endif

