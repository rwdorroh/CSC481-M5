#include <engine/GameObjectAllocator.hpp>

GameObjectAllocator::AllocMode GameObjectAllocator::allocationMode = GameObjectAllocator::DYNAMIC;
std::unique_ptr<GameObjectPool> GameObjectAllocator::pool = nullptr;

GameObject* GameObjectAllocator::create() {
    if (allocationMode == POOLED && pool) {
        return pool->allocate();
    }
    return new GameObject(); // Dynamic fallback
}

void GameObjectAllocator::destroy(GameObject* obj) {
    if (!obj) return;
    
    if (allocationMode == POOLED && pool && pool->owns(obj)) {
        pool->deallocate(obj);
    } else {
        delete obj;
    }
}