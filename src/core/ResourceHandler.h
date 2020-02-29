#pragma once

#include "core/pch.h"
#include "core/util/FileUtils.h"
#include "core/renderer/Texture.h"

class ResourceBase;
class ResourceStorageBase;

template <typename T>
class Resource;

template <typename T>
class ResourceStorage;

class ResourceHandler;

template <typename T>
struct ResourceLoader;

enum class ResourceStatus {
	Ready = 0, // The resource is ready to use
	Requested = 1, // The resource has been requested but is not ready
	Failed = Requested | 2, // The resource has been requested and failed
	TimedOut = Failed | 4, // The resource has been requested and failed because the request timed out
	NotFound = Failed | 8, // The resource has been requested and failed because it was not found
	Invalid = 16, // The resource has been invalidated
};

class ResourceBase {
public:

protected:
	virtual ~ResourceBase() = default;
};

class ResourceStorageBase {
public:
	virtual bool contains(std::string id) = 0;

protected:
	virtual ~ResourceStorageBase() = default;
};


template <typename T>
class Resource : public ResourceBase {
	friend class ResourceStorage<T>;
	friend class ResourceHandler;
public:
	Resource();

	T& operator*();

	T* operator->() const;

	ResourceStatus status() const;

	bool exists() const;

	bool operator==(const T* other) const;

	bool operator!=(const T* other) const;

	~Resource();

private:
	Resource(ResourceStatus status, ResourceStorage<T>* storage = NULL, T* data = NULL);

	Resource<T>& get();

	uint32_t m_references;
	ResourceStatus m_status;
	ResourceStorage<T>* m_storage;
	T* m_data;
};


template <typename T>
class ResourceStorage : public ResourceStorageBase {
	friend class ResourceHandler;
public:
	using resource_map = typename std::unordered_map<std::string, Resource<T>*>;
	using resource_iterator = typename resource_map::iterator;
	using const_resource_iterator = typename resource_map::const_iterator;

	// TODO async resource loading. Return "ResourceRequest<T>" or something similar

	Resource<T> get(std::string id);

	Resource<T> request(std::string id, std::string url = "");

	void release(Resource<T>* resourcePtr);

	bool contains(std::string id) override;

private:
	bool load(std::string url, T** dataPtr);

	ResourceStorage(ResourceHandler* resourceHandler, ResourceLoader<T>* resourceLoader);

	~ResourceStorage();

	ResourceHandler* m_resourceHandler;
	ResourceLoader<T>* m_resourceLoader;
	resource_map m_resources;
};



class ResourceHandler {
public:
	using storage_map = typename std::unordered_map<std::type_index, ResourceStorageBase*>;
	using storage_iterator = storage_map::iterator;
	using const_storage_iterator = storage_map::const_iterator;

	ResourceHandler(std::string resourceDirectory);

	~ResourceHandler();

	template <typename T>
	ResourceStorage<T>* getStorage();

	template <typename T>
	ResourceStorage<T>* setLoader(ResourceLoader<T>* loader);

	template <typename T>
	Resource<T> get(std::string id);

	template <typename T>
	Resource<T> request(std::string id, std::string url = "");

	template <typename T>
	void release(Resource<T>* resourcePtr);

	template <typename T>
	bool hasStorage() const;

	// check any resource corresponding to id exists
	bool contains(std::string id) const;

	// check if any resource of the specified type corresponding to id exists
	template <typename T>
	bool contains(std::string id) const;

	std::string getResourceDirectory() const;

private:
	std::string m_resourceDirectory;
	storage_map m_resourceStorage;
};



#ifndef RESOURCE_LOADERS_IMPL
#define RESOURCE_LOADERS_IMPL

template<typename T>
inline Resource<T>::Resource(ResourceStatus status, ResourceStorage<T>* storage, T* data) {
	m_status = status;
	m_storage = storage;
	m_data = data;
}

template<typename T>
inline Resource<T>::~Resource() {

}

template<typename T>
inline Resource<T>& Resource<T>::get() {
	m_references++;
	return *this;
}

template<typename T>
inline Resource<T>::Resource() {
	m_status = ResourceStatus::Invalid;
	m_storage = NULL;
	m_data = NULL;
}

template<typename T>
inline T& Resource<T>::operator*() {
	assert(this->m_status == ResourceStatus::Ready);
	return *m_data;
}

template<typename T>
inline T* Resource<T>::operator->() const {
	assert(this->m_status == ResourceStatus::Ready);
	return m_data;
}

template<typename T>
inline ResourceStatus Resource<T>::status() const {
	return m_status;
}

template<typename T>
inline bool Resource<T>::exists() const {
	return (*this) != NULL && m_status == ResourceStatus::Ready;
}

template<typename T>
inline bool Resource<T>::operator==(const T* other) const {
	if (m_status != ResourceStatus::Ready && other == NULL) {
		return true;
	}

	return m_status == ResourceStatus::Ready && m_data == other;
}

template<typename T>
inline bool Resource<T>::operator!=(const T* other) const {
	return !this->operator==(other);
}



template<typename T>
inline Resource<T> ResourceStorage<T>::get(std::string id) {
	resource_iterator it = m_resources.find(id);
	if (it == m_resources.end()) {
		return Resource<T>(ResourceStatus::Invalid);
	}

	return it->second->get();
}

template<typename T>
inline Resource<T> ResourceStorage<T>::request(std::string id, std::string url) {
	std::pair<resource_iterator, bool> result = m_resources.emplace(id, new Resource<T>(ResourceStatus::Requested, this, NULL));
	//info("Requesting resource \"%s\"\n", id.c_str());

	if (result.second) { // id does not already exist
		//info("Loading resource \"%s\" from \"%s\"\n", id.c_str(), url.c_str());

		T* data = NULL;
		if (!this->load(url, &data)) {
			result.first->second->m_status = ResourceStatus::Failed;
		} else {
			result.first->second->m_status = ResourceStatus::Ready;
			result.first->second->m_data = data;
		}
	}

	return result.first->second->get();
}

template<typename T>
inline void ResourceStorage<T>::release(Resource<T>* resourcePtr) {
	assert(resourcePtr != NULL);

	Resource<T> resource = *resourcePtr;
	resource.m_references--;

	if (resource.m_references <= 0) {
		//delete resource.m_data;
		//resource.m_data = NULL;
		//resource.m_status = ResourceStatus::Invalid;
	}

	*resourcePtr = Resource<T>(ResourceStatus::Invalid);
}

template<typename T>
inline bool ResourceStorage<T>::contains(std::string id) {
	return this->get(id).status() != ResourceStatus::Invalid;
}

template<typename T>
inline bool ResourceStorage<T>::load(std::string url, T** dataPtr) {
	return (*m_resourceLoader)(m_resourceHandler->getResourceDirectory() + "/" + url, dataPtr);
}

template<typename T>
inline ResourceStorage<T>::ResourceStorage(ResourceHandler* resourceHandler, ResourceLoader<T>* resourceLoader) :
	m_resourceHandler(resourceHandler),
	m_resourceLoader(resourceLoader) {
}

template<typename T>
inline ResourceStorage<T>::~ResourceStorage() {

}






inline ResourceHandler::ResourceHandler(std::string resourceDirectory) :
	m_resourceDirectory(resourceDirectory) {}

inline ResourceHandler::~ResourceHandler() {

}

template<typename T>
inline ResourceStorage<T>* ResourceHandler::getStorage() {
	std::type_index type = typeid(T);

	storage_iterator it = m_resourceStorage.find(type);
	if (it == m_resourceStorage.end()) {
		return NULL;
	}

	return dynamic_cast<ResourceStorage<T>*>(it->second);
}

template<typename T>
inline ResourceStorage<T>* ResourceHandler::setLoader(ResourceLoader<T>* loader) {
	std::type_index type = typeid(T);

	std::pair<storage_iterator, bool> result = m_resourceStorage.emplace(type, new ResourceStorage<T>(this, loader));
	ResourceStorage<T>* storage = dynamic_cast<ResourceStorage<T>*>(result.first->second);
	assert(storage != NULL);

	if (!result.second) { // result already exists
		storage->m_resourceLoader = loader;
	} else {
		info("Initialized resource storage for %s\n", typeid(T).name());
	}

	return storage;
}

template<typename T>
inline Resource<T> ResourceHandler::get(std::string id) {
	ResourceStorage<T>* storage = this->getStorage<T>();
	if (storage == NULL) {
		return Resource<T>(ResourceStatus::Invalid);
	}

	return storage->get(id);
}

template<typename T>
inline Resource<T> ResourceHandler::request(std::string id, std::string url) {
	ResourceStorage<T>* storage = this->getStorage<T>();
	if (storage == NULL) {
		return Resource<T>(ResourceStatus::Invalid);
	}

	return storage->request(id, url);
}

template<typename T>
inline void ResourceHandler::release(Resource<T>* resourcePtr) {
	assert(resourcePtr != NULL);

	ResourceStorage<T>* storage = this->getStorage<T>();
	if (storage == NULL) {
		warn("Cannot release resource - missing resource storage\n");
		return;
	}

	storage->release(resourcePtr);
}

template<typename T>
inline bool ResourceHandler::hasStorage() const {
	return this->getStorage<T>() != NULL;
}

inline bool ResourceHandler::contains(std::string id) const {
	for (const_storage_iterator it = m_resourceStorage.begin(); it != m_resourceStorage.end(); it++) {
		if (it->second->contains(id)) {
			return true;
		}
	}

	return false;
}

template <typename T>
inline bool ResourceHandler::contains(std::string id) const {
	ResourceStorage<T>* storage = this->getStorage<T>();
	if (storage == NULL) {
		return false;
	}

	return storage->contains(id);
}

inline std::string ResourceHandler::getResourceDirectory() const {
	return m_resourceDirectory;
}




template <typename T>
struct ResourceLoader {
	virtual bool operator()(std::string url, T** data) = 0;
};

namespace ResourceLoaders {
	struct TextLoader : public ResourceLoader<std::string> {
		inline virtual bool operator()(std::string url, std::string** data) override {
			if (url == "" || data == NULL)
				return false;

			*data = new std::string();
			return FileUtils::loadFile(url, **data);
		}
	};

	struct ImageLoader : public ResourceLoader<Image> {
		inline virtual bool operator()(std::string url, Image** data) override {
			if (url == "" || data == NULL)
				return false;

			*data = new Image();
			return FileUtils::loadImage(url, **data);
		}
	};

	struct Texture2DLoader : public ResourceLoader<Texture2D> {
		inline virtual bool operator()(std::string url, Texture2D** data) override {
			if (url == "" || data == NULL)
				return false;

			return Texture2D::load(url, data);
		}
	};
};

#endif