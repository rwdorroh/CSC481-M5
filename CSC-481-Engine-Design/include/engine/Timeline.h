#pragma once

#include <SDL3/SDL.h>

class Timeline {
public:
    // Initialize internal tick tracking
    void init();

    // Update internal time and return scaled deltaTime
    double update();

    // Set the speed scale, clamped between minSpeed and maxSpeed
    void setScale(double s);
    double getScale() const;

    // Pause/unpause the timeline
    void pause();
    void resume();
    bool isPaused() const;

    // Total accumulated game time (scaled)
    double getAccumulatedTime() const;

private:
    double timeScale = 1.0;
    double accumulated = 0.0;
    Uint64 lastTicks = 0;
    bool paused = false;

    static constexpr double doubleSpeed = 2.0;
    static constexpr double halfSpeed = 0.5;
};
