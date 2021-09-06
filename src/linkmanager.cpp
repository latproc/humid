#include "linkmanager.h"
#include "linkableproperty.h"
#include <map>
#include <list>

LinkManager * LinkManager::_instance = nullptr;

class LinkManager::Impl {
public:
    std::map<std::string, LinkableProperty *> properties;
    std::map<std::string, std::map<std::string, std::list<LinkInfo> > > pending_links;

    ~Impl() {
        for (auto & item : properties) {
            delete item.second;
        }
    }

    bool has_property(const std::string &property) const {
        return properties.find(property) != properties.end();
    }

    void add(const std::string &property, LinkableProperty *links) {
        auto found = properties.find(property);
        if (found != properties.end()) {
            std::cout << "TODO: add extra links to a linkable property\n";
        }
        else {
            properties[property] = links;
        }
    }

    LinkableProperty *links(const std::string &property) {
        auto found = properties.find(property);
        if (found!= properties.end()) return (*found).second;
        return nullptr;
    }

    void add_pending(const std::string &remote, const std::string class_name, const std::string & widget_name, const std::string &property) {
        std::cout << "add pending " << remote << "->" << property << "\n";
        auto found_class = pending_links.find(class_name);
        if (found_class != pending_links.end()) {
            auto & class_pending_properties = (*found_class).second;
            auto found_widget = class_pending_properties.find(widget_name);
            if (found_widget != class_pending_properties.end()) {
                auto property_list = (*found_widget).second;
                property_list.push_back({remote, property});
            }
            else {
                class_pending_properties[widget_name] = std::list<LinkInfo>();
                class_pending_properties[widget_name].push_back({remote, property});
            }
        }
        else {
            pending_links[class_name] = std::map<std::string, std::list<LinkInfo> >();
            pending_links[class_name][widget_name] = std::list<LinkInfo>();
            pending_links[class_name][widget_name].push_back({remote, property});
        }
    }

    std::list<LinkInfo> *remote_links(const std::string & class_name, const std::string & widget_name) {
        auto found_class = pending_links.find(class_name);
        if (found_class != pending_links.end()) {
            auto & class_pending_properties = (*found_class).second;
            auto found_widget = class_pending_properties.find(widget_name);
            if (found_widget != class_pending_properties.end()) {
                return &(*found_widget).second;
            }
        }
        return nullptr;
    }

};

LinkManager::LinkManager() : impl{new Impl} {
}

LinkManager & LinkManager::instance() {
    if (!_instance){
        _instance = new LinkManager;
    }
    return *_instance;
}

bool LinkManager::has_property(const std::string &property) const {
    return impl->has_property(property);
}
void LinkManager::add(const std::string &property, LinkableProperty *links) {
    impl->add(property, links);
}

LinkableProperty *LinkManager::links(const std::string &property) const {
    return impl->links(property);
}

void LinkManager::remove() {
    delete impl;
    delete this;
}

void LinkManager::add_pending(const std::string &remote_name, const std::string &class_name, const std::string &widget_name, const std::string &property) {
    impl->add_pending(remote_name, class_name, widget_name, property);
}

std::list<LinkManager::LinkInfo> *LinkManager::remote_links(const std::string & class_name, const std::string & widget_name) {
    return impl->remote_links(class_name, widget_name);
}