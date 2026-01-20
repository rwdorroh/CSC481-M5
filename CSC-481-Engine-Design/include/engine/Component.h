#pragma once
#include <SDL3/SDL.h>

class GameObject; // Forward declaration

class Component {
public:
    virtual ~Component() = default;

    // Called every frame to update component behavior
    virtual void update(GameObject& obj, float deltaTime) {}

    // Called every frame to render (optional override)
    virtual void draw(GameObject& obj, SDL_Renderer* renderer) {}

    // Optional init hook when added to an object
    virtual void onAdd(GameObject& obj) { this->owner = &obj; }

protected:
    GameObject* owner = nullptr;
};
