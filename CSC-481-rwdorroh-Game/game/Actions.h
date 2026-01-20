#pragma once
#include <cstdint>

// Bitmask actions that travel over the network
constexpr uint32_t ACTION_JUMP = 1 << 0;
constexpr uint32_t ACTION_DODGE = 1 << 1;
constexpr uint32_t ACTION_SCALE_UP = 1 << 2;
constexpr uint32_t ACTION_SCALE_DOWN = 1 << 3;
constexpr uint32_t ACTION_PAUSE = 1 << 4;
constexpr uint32_t ACTION_LEFT = 1 << 5;
constexpr uint32_t ACTION_RIGHT = 1 << 6;
// (add more up to 31 as needed)
