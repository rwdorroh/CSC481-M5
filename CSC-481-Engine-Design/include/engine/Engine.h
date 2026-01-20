#pragma once

#include "GameObject.h"
#include <SDL3/SDL.h>
#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

// The core engine class. It manages the game loop, window, renderer, and entities.
class Engine {
public:
    // This is the corrected Config struct, nested inside the Engine class.
    struct Config {
        const char* title;
        int width;
        int height;
    };
    
	// Runs the main game loop.
	static void run(std::function<void(float)> update, std::function<void(void)> render);

	// GameObject management
	static void addGameObject(GameObject* obj);
	static void removeGameObject(GameObject* obj);
	static void queueRemove(GameObject* obj);
	static std::vector<GameObject*> getGameObjectsSnapshot();
	static std::mutex& getGameObjectsMutex();

    // Memory pool configuration
    static void usePoolAllocator(bool usePool, size_t poolCapacity = 100);
    
    // Create GameObject with current allocator
    static GameObject* createGameObject();
    
    // Get pool statistics
    static float getPoolUsagePercent();
    static size_t getPoolCapacity();
    
	// Getter for the renderer.
	static SDL_Renderer* getRenderer();

    // Initializes the engine
	static bool init(const Config& cfg);

    // Shuts down the engine and cleans up all resources.
	static void shutdown();


private:
	// Private members for the engine's core functionality.
	static SDL_Window* s_window;
	static SDL_Renderer* s_renderer;
	static bool s_running;

	// Game object tracking
	static std::vector<GameObject*> s_gameObjects;
	static std::mutex s_gameObjectsMutex;
	static std::vector<GameObject*> s_pendingRemovals;

	// Multithreading private members
	static std::thread s_updateThread;
	static std::atomic<bool> s_workerRunning;
};
