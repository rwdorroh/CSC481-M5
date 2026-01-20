#include <engine/GameObjectPool.hpp>
#include <new>

GameObjectPool::GameObjectPool(size_t capacity)
    : capacity(capacity), usedCount(0) {
    memory = static_cast<char*>(::operator new(capacity * sizeof(GameObject)));
    used = new bool[capacity];
    std::memset(used, 0, capacity * sizeof(bool));
}

GameObjectPool::~GameObjectPool() {
    for (size_t i = 0; i < capacity; i++) {
        if (used[i]) {
            GameObject* obj = reinterpret_cast<GameObject*>(memory + i * sizeof(GameObject));
            obj->~GameObject();
        }
    }
    ::operator delete(memory);
    delete[] used;
}

GameObject* GameObjectPool::allocate() {
    for (size_t i = 0; i < capacity; i++) {
        if (!used[i]) {
            used[i] = true;
            usedCount++;
            GameObject* ptr = reinterpret_cast<GameObject*>(memory + i * sizeof(GameObject));
            new (ptr) GameObject();
            return ptr;
        }
    }
    return nullptr;
}

void GameObjectPool::deallocate(GameObject* obj) {
    char* objMem = reinterpret_cast<char*>(obj);
    ptrdiff_t diff = objMem - memory;
    size_t index = diff / sizeof(GameObject);

    if (index >= 0 && index < capacity && used[index]) {
        obj->~GameObject();
        used[index] = false;
        usedCount--;
    }
}

bool GameObjectPool::owns(GameObject* obj) const {
    char* objMem = reinterpret_cast<char*>(obj);
    return objMem >= memory && objMem < memory + (capacity * sizeof(GameObject));
}

float GameObjectPool::getUsagePercent() const {
    return capacity > 0 ? (usedCount * 100.0f / capacity) : 0.0f;
}