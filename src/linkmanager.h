#pragma once

#include <string>
#include "linkableproperty.h"
#include "symboltable.h"

class EditorWidget;
class LinkManager {
public:
    struct LinkInfo {
        std::string remote_name;
        std::string property_name;
    };
    using Links = std::list<LinkInfo>;

    static LinkManager &instance();
    void remove();

    bool has_property(const std::string &property) const;
    void add(const std::string &property, LinkableProperty *links);
    LinkableProperty *links(const std::string &property) const;
 
    void add_pending(const std::string &remote_name, const std::string & class_name, const std::string & widget_name, const std::string & property);
    Links *remote_links(const std::string & class_name, const std::string & widget_name);

    //void unlink(const std::string & class_name, EditorWidget *widget);

private:
    LinkManager();
    ~LinkManager();
    class Impl;
    static LinkManager *_instance;
    Impl *impl = nullptr;
};
