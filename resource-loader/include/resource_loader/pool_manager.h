#ifndef GRAPHICS_POOL_MANAGER_H
#define GRAPHICS_POOL_MANAGER_H

#include <vector>
#include <graphics/resources.h>
#include <stdexcept>

#include "texture_loader.h"

class BasePoolManager {
public:
    InternalTexLoader* tex(Resource::Texture tex) {
	return this->tex(tex.pool);
    }

    InternalTexLoader* tex(Resource::Pool pool) {
	return tex(pool.ID);
    }
    
    virtual InternalTexLoader* tex(int id) = 0;
};

template<class Pool>
class InternalPoolManager : public BasePoolManager {
 public:
    ~InternalPoolManager();
    bool ValidPool(Resource::Pool pool);
    bool ValidPool(int id);
    Pool* get(Resource::Pool pool);
    Pool* get(int id);
    int PoolCount();
    int NextPoolIndex();
    Pool* AddPool(Pool* pool, int index);
    void DeletePool(Resource::Pool pool);
    
 protected:
    std::vector<Pool*> pools;
    std::vector<int> freePools;
};


// assume <graphics/logger.h> imported
// and pools has a member called texLoader
#define MAKE_POOL_MANAGER(name, resource_pool_type) \
    class name : public InternalPoolManager<resource_pool_type> {	\
	InternalTexLoader* tex(int id) override {			\
	    if(!ValidPool(id)) {					\
		LOG_ERROR("PoolManager: tex given invalid pool id: "	\
			  << id);					\
		return nullptr;						\
	    }								\
	    return pools[id]->texLoader;				\
	}								\
    };

/// PoolManager Implementation

template <class Pool>
InternalPoolManager<Pool>::~InternalPoolManager() {
    for(int i = 0; i < pools.size(); i++)
	if(pools[i] != nullptr)
	    delete pools[i];
   
}


template <class Pool>
bool InternalPoolManager<Pool>::ValidPool(Resource::Pool pool) {
    return ValidPool(pool.ID);
}


template <class Pool>
bool InternalPoolManager<Pool>::ValidPool(int id) {
    return !(id > pools.size() || pools[id] == nullptr);
}


template <class Pool>
Pool* InternalPoolManager<Pool>::get(Resource::Pool pool) {
    return get(pool.ID);
}


template <class Pool>
Pool* InternalPoolManager<Pool>::get(int id) {
    if(!ValidPool(id))
	return nullptr;
    return pools[id];
}


template <class Pool>
int InternalPoolManager<Pool>::PoolCount() {
    return pools.size();
}


template <class Pool>
int InternalPoolManager<Pool>::NextPoolIndex() {
    int index = pools.size();
    if(freePools.empty())
	pools.push_back(nullptr);
    else {
	index = freePools.back();
	freePools.pop_back();
    }
    return index;
}


template <class Pool>
Pool* InternalPoolManager<Pool>::AddPool(Pool* pool, int index) {
    if(ValidPool(index)) {
	throw std::runtime_error("PoolManager: Tried to add pool to index "
				 "that already contains a valid pool!");
    }
    pools[index] = pool;
    return pools[index];
}


template <class Pool>
void InternalPoolManager<Pool>::DeletePool(Resource::Pool pool) {
    if(!ValidPool(pool))
	return;
    delete pools[pool.ID];
    if(pools.size() == pool.ID + 1) {
	pools.pop_back();
	while(pools.size() > 0 && pools[pools.size() -1] == nullptr) {
	    for(int i = freePools.size() - 1; i >= 0; i--) {
		if(freePools[i] == pools.size() - 1) {
		    freePools.erase(freePools.begin() + i);
		    break;
		}
	    }
	    pools.pop_back();
	}
    } else {
	pools[pool.ID] = nullptr;
	freePools.push_back(pool.ID);
    }
}


#endif 
