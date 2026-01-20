#pragma once
#include "Component.h"
#include <SDL3/SDL.h>

struct Vec2 {
    float x;
    float y;
};

class TransformComponent : public Component {
public:
    TransformComponent(float x = 0.f, float y = 0.f, float w = 32.f, float h = 32.f, float vx = 0.f, float vy = 0.f)
        : position({ x, y }), size({ w, h }), velocity({ vx, vy }) {
    }

    // Position accessors
    Vec2 getPosition() const { return position; }
    void setPosition(float x, float y) { position = { x, y }; }

    // Size accessors
    Vec2 getSize() const { return size; }
    void setSize(float w, float h) { size = { w, h }; }

    // Velocity accessors
    Vec2 getVelocity() const { return velocity; }
    void setVelocity(float vx, float vy) { velocity = { vx, vy }; }

    // Move by velocity * deltaTime
    void update(GameObject& obj, float deltaTime) override {
        position.x += velocity.x * deltaTime;
        position.y += velocity.y * deltaTime;
    }

    // Helper to zero out velocity for static objects
    void stop() { velocity = { 0.f, 0.f }; }

private:
    Vec2 position;   // world position
    Vec2 size;       // dimensions
    Vec2 velocity;   // movement velocity
};
