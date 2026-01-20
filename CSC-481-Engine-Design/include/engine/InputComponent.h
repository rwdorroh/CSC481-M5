#pragma once

#include "Component.h"
#include <engine/Input.h>
#include <cstdint>

class InputComponent : public Component {
public:
    InputComponent() = default;

    // Polls the Input system once per frame and stores the bitmask.
    void update(GameObject& obj, float) override {
        actionMask = Input::getActionMask();
    }

    // Returns the current input bitmask for this frame.
    uint32_t getActionMask() const { return actionMask; }

private:
    uint32_t actionMask = 0;
};
