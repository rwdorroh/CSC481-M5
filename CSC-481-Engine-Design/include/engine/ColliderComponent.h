#pragma once
#include "Component.h"

class ColliderComponent : public Component {
public:
    ColliderComponent(bool enabled = true)
        : collidable(enabled) {
    }

    bool isCollidable() const { 
        return collidable; 
    }

    void setCollidable(bool value) { 
        collidable = value; 
    }

private:
    bool collidable;
};
