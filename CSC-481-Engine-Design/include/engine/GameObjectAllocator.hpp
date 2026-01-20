#pragma once
#include "GameObjectPool.hpp"
#include <memory>

class GameObjectAllocator {
public:
    enum AllocMode {
        DYNAMIC,   // Use new/delete
        POOLED     // Use pool allocator
    };

    static void setMode(AllocMode mode) { allocationMode = mode; }
    static void setPoolCapacity(size_t capacity) {
        if (!pool) {
            pool = std::make_unique<GameObjectPool>(capacity);
        }
    }

    static GameObject* create();
    static void destroy(GameObject* obj);
    
    static float getPoolUsagePercent() {
        return pool ? pool->getUsagePercent() : 0.0f;
    }
    static size_t getPoolCapacity() {
        return pool ? pool->getCapacity() : 0;
    }

private:
    static AllocMode allocationMode;
    static std::unique_ptr<GameObjectPool> pool;
};