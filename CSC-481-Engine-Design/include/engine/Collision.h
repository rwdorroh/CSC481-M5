#pragma once

#include "GameObject.h"
#include "TransformComponent.h"
#include "ColliderComponent.h"
#include <SDL3/SDL.h>

class Collision {
public:
    static bool checkCollision(GameObject& a, GameObject& b) {

        auto* ta = a.getComponent<TransformComponent>();
        auto* tb = b.getComponent<TransformComponent>();
        auto* ca = a.getComponent<ColliderComponent>();
        auto* cb = b.getComponent<ColliderComponent>();

        if (!ta || !tb || !ca || !cb) return false;
        if (!ca->isCollidable() || !cb->isCollidable()) return false;

        SDL_FRect rectA{ ta->getPosition().x, ta->getPosition().y, ta->getSize().x, ta->getSize().y };
        SDL_FRect rectB{ tb->getPosition().x, tb->getPosition().y, tb->getSize().x, tb->getSize().y };

        bool xOverlap = (rectA.x < rectB.x + rectB.w) && (rectA.x + rectA.w > rectB.x);
        bool yOverlap = (rectA.y < rectB.y + rectB.h) && (rectA.y + rectA.h > rectB.y);

        return xOverlap && yOverlap;
    }
};
