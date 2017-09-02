//
//  ResourceManager.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __resourcemanager_h__
#define __resourcemanager_h__

#include <ostream>
#include <string>

#include <map>
#include <assert.h>
#include <iostream>

class ResourceManager {
	int item_id;
	int refs;
	static std::map<int, ResourceManager*> resources;
protected:
	~ResourceManager();
	
public:	
	
	ResourceManager();
	ResourceManager(int resource, int init_refs = 1);
	
	
	class Factory {
		public:
		~Factory();
		virtual ResourceManager *create() const;
		virtual ResourceManager *create(int item_id, int refs=1) const;
	};

	void use();
	void release();
	virtual void close(int which);
	
	static ResourceManager *find(int item);
	
	static int manage(int resource, const ResourceManager::Factory &factory = default_factory);
	
	static int release(int resource);
	
	static size_t size();
	
private:
	static ResourceManager::Factory default_factory;
};

class TextureResourceManager : public ResourceManager {
public:
	TextureResourceManager();
	TextureResourceManager(int texture_id, int init_refs = 0);
	void close(int texture_id);
};

class TextureResourceManagerFactory : public ResourceManager::Factory {
public:	
	virtual ResourceManager *create() const;
	virtual ResourceManager *create(int item_id, int refs=1) const;
};

#endif