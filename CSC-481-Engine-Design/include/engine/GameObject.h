#pragma once

#include <memory>
#include <map>
#include <string>
#include <typeindex>
#include <typeinfo>

#include <SDL3/SDL.h>
#include "Component.h"
#include "RenderComponent.h"

class GameObject {
public:
    void setPaused(bool p) { paused = p; }
    bool isPaused() const { return paused; }

    GameObject() = default;
    ~GameObject() = default;

    // Add a component of type T
    template <typename T>
    void addComponent(std::unique_ptr<T> comp) {
        std::type_index key(typeid(T));
        comp->onAdd(*this);  // Inform the component of its owner
        components[key] = std::move(comp);
    }
	
	// Remove a component of type T
	template <typename T>
	void removeComponent() {
		std::type_index key(typeid(T));
		auto it = components.find(key);
		if (it != components.end()) {
			components.erase(it);
		}
	}

	// Has component function
	template <typename T>
	bool hasComponent() const {
		std::type_index key(typeid(T));
		return components.find(key) != components.end();
	}

    // Get component of type T, or nullptr if not present
    template <typename T>
    T* getComponent() {
        std::type_index key(typeid(T));
        auto it = components.find(key);
        if (it != components.end()) {
            return dynamic_cast<T*>(it->second.get());
        }
        return nullptr;
    }

    // Update all components
    void update(float deltaTime) {
    if (paused) return;

    for (auto& [_, comp] : components) {
        comp->update(*this, deltaTime);
    }
}

    // Draw all components
	void draw(SDL_Renderer* renderer) {
		auto* renderComp = getComponent<RenderComponent>();
		if (renderComp) {
			renderComp->draw(*this, renderer);
		}
	}


	bool isRenderable() const {
		return hasComponent<RenderComponent>();
	}

private:
    std::map<std::type_index, std::unique_ptr<Component>> components;
    bool paused = false;
};
