#pragma once

#include "Component.h"
#include "TransformComponent.h"
#include <cmath>
#include "GameObject.h"

class GravityComponent : public Component {
public:
    GravityComponent(float gravity = 9.81f)
        : gravityStrength(gravity) {
    }

    void update(GameObject& obj, float deltaTime) override {
    if (obj.isPaused()) return;
    if (deltaTime <= 0.0f) return;

    auto* transform = obj.getComponent<TransformComponent>();
    if (!transform) return;

    auto velocity = transform->getVelocity();
    velocity.y += gravityStrength * deltaTime;
    transform->setVelocity(velocity.x, velocity.y);
}

private:
    float gravityStrength;
};