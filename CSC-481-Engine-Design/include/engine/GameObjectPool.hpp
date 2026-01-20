#pragma once
#include "GameObject.h"
#include <cstring>

class GameObjectPool {
public:
    GameObjectPool(size_t capacity);
    ~GameObjectPool();

    GameObjectPool(const GameObjectPool&) = delete;
    GameObjectPool& operator=(const GameObjectPool&) = delete;

    GameObject* allocate();
    void deallocate(GameObject* obj);
    bool owns(GameObject* obj) const;

    float getUsagePercent() const;
    size_t getCapacity() const { return capacity; }
    size_t getUsedCount() const { return usedCount; }

private:
    size_t capacity;
    size_t usedCount;
    char* memory;
    bool* used;
};