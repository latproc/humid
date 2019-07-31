//
//  ResourceManager.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include "resourcemanager.h"
#include <nanogui/common.h>
#include <nanogui/opengl.h>
#include <nanogui/glutil.h>
#include <nanovg_gl.h>
#include <lib_clockwork_client.hpp>


std::map<int, ResourceManager*> ResourceManager::resources;
ResourceManager::Factory ResourceManager::default_factory;

ResourceManager::~ResourceManager() { }

ResourceManager::ResourceManager() : item_id(0), refs(0), last_release_time(0) {}

ResourceManager::ResourceManager(int resource, int init_refs) : item_id(resource), refs(init_refs) {
    resources.insert(std::make_pair(resource, this));
}

ResourceManager::Factory::~Factory() {}

ResourceManager *ResourceManager::Factory::create() const { return new ResourceManager(); }

ResourceManager *ResourceManager::Factory::create(int item_id, int refs) const {
    return new ResourceManager(item_id, refs);
}

void ResourceManager::use() { ++refs; }

int ResourceManager::release() {
    assert(refs);
    last_release_time = microsecs();
    if (--refs == 0) {
        resources.erase(item_id);
        close(item_id);
        delete this;
        return 0;
    }
    return refs;
}

void ResourceManager::close(int which) {
}

ResourceManager *ResourceManager::find(int item) {
    std::map<int, ResourceManager*>::iterator found = resources.find(item);
    if (found == resources.end()) return 0;
    return (*found).second;
}

int ResourceManager::manage(int resource, const ResourceManager::Factory &factory) {
    ResourceManager *manager = ResourceManager::find(resource);
    if (!manager) {
        manager = factory.create(resource);
    }
    else
        manager->use();
    return resource;
}

int ResourceManager::handover(int resource, const ResourceManager::Factory &factory) {
    auto found = resources.find(resource);
    if (found != resources.end()) {
        ResourceManager *manager = (*found).second;
        resources.erase(found);
        int refs = manager->uses();
        delete manager;
        factory.create(resource, refs);
    }
    else
        factory.create(resource);
    return resource;
}

int ResourceManager::release(int resource) {
    if (!resource) return 0;
    ResourceManager *manager = ResourceManager::find(resource);
    return (manager) ? manager->release() : 0;
}

size_t ResourceManager::size() { return resources.size(); }

TextureResourceManager::TextureResourceManager() : ResourceManager() { }
TextureResourceManager::TextureResourceManager(int texture_id, int init_refs) : ResourceManager(texture_id, init_refs) { }
void TextureResourceManager::close(int texture_id) {
    GLuint id = texture_id;
    if (glIsTexture(id)) {
        glDeleteTextures(1, &id);
    }
}

ResourceManager *TextureResourceManagerFactory::create() const { return new TextureResourceManager(); }
ResourceManager *TextureResourceManagerFactory::create(int item_id, int refs) const { return new TextureResourceManager(item_id, refs); }

#ifdef TESTING

int main (int argc, char *argv[]) {
	TextureResourceManagerFactory resource_manager_factory;
	int tex = ResourceManager::manage(1, resource_manager_factory);
	int tex1 = ResourceManager::manage(1, resource_manager_factory);
	int tex2 = ResourceManager::manage(2, resource_manager_factory);
	int tex3 = ResourceManager::manage(2, resource_manager_factory);

	std::cout << "Active textures: " << ResourceManager::size() << "\n";
	tex = ResourceManager::release(tex);
	tex3 = ResourceManager::release(tex3);
	tex1 = ResourceManager::release(tex1);
	tex2 = ResourceManager::release(tex2);
	std::cout << "Active textures: " << ResourceManager::size() << "\n";

	return 0;
}

#endif
