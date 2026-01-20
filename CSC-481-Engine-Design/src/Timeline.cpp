#include "engine/Timeline.h"
#include <algorithm>

// Initialize the timeline by capturing the current ticks
void Timeline::init() {
    lastTicks = SDL_GetTicks();
}

// Update the timeline, returning the scaled delta time since last call
double Timeline::update() {
    if (paused) return 0.0;

    Uint64 now = SDL_GetTicks();
    double delta = (now - lastTicks) / 1000.0;
    lastTicks = now;
    accumulated += delta * timeScale;
    return delta * timeScale;
}

// Clamp scale between halfSpeed and doubleSpeed
void Timeline::setScale(double s) {
    timeScale = std::clamp(s, halfSpeed, doubleSpeed);
}

// Get the current time scale
double Timeline::getScale() const {
    return timeScale;
}

// Pause the timeline
void Timeline::pause() {
    paused = true;
}

// Resume the timeline
void Timeline::resume() {
    paused = false;
    lastTicks = SDL_GetTicks(); // avoid time jump on resume
}

// Check if the timeline is paused
bool Timeline::isPaused() const {
    return paused;
}

// Get the total accumulated game time (scaled)
double Timeline::getAccumulatedTime() const {
    return accumulated;
}
