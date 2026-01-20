#include <engine/Engine.h>
#include <engine/Input.h>
#include <engine/RenderComponent.h>
#include <engine/GameObjectAllocator.hpp>
#include <SDL3/SDL.h>
#include <iostream>
#include <algorithm>
#include <unordered_set>

SDL_Window* Engine::s_window = nullptr;
SDL_Renderer* Engine::s_renderer = nullptr;
bool Engine::s_running = false;

std::vector<GameObject*> Engine::s_gameObjects;
std::mutex Engine::s_gameObjectsMutex;

std::thread Engine::s_updateThread;
std::atomic<bool> Engine::s_workerRunning = false;

std::vector<GameObject*> Engine::s_pendingRemovals;

bool Engine::init(const Config& cfg) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return false;
    }

    s_window = SDL_CreateWindow(cfg.title, cfg.width, cfg.height, SDL_WINDOW_RESIZABLE);
    if (!s_window) {
        SDL_Log("Couldn't create window: %s", SDL_GetError());
        return false;
    }

    s_renderer = SDL_CreateRenderer(s_window, nullptr);
    if (!s_renderer) {
        SDL_Log("Couldn't create renderer: %s", SDL_GetError());
        return false;
    }

    return true;
}

void Engine::shutdown() {
    s_workerRunning = false;
    if (s_updateThread.joinable()) s_updateThread.join();

    {
        std::lock_guard<std::mutex> lock(s_gameObjectsMutex);
        for (GameObject* obj : s_gameObjects) {
            GameObjectAllocator::destroy(obj);
        }
        s_gameObjects.clear();
    }

    SDL_DestroyRenderer(s_renderer);
    SDL_DestroyWindow(s_window);
    SDL_Quit();
}

void Engine::addGameObject(GameObject* obj) {
    std::lock_guard<std::mutex> lock(s_gameObjectsMutex);
    s_gameObjects.push_back(obj);
}

void Engine::usePoolAllocator(bool usePool, size_t poolCapacity) {
    if (usePool) {
        GameObjectAllocator::setMode(GameObjectAllocator::POOLED);
        GameObjectAllocator::setPoolCapacity(poolCapacity);
    } else {
        GameObjectAllocator::setMode(GameObjectAllocator::DYNAMIC);
    }
}

GameObject* Engine::createGameObject() {
    GameObject* obj = GameObjectAllocator::create();
    if (obj) {
        addGameObject(obj);
    }
    return obj;
}

float Engine::getPoolUsagePercent() {
    return GameObjectAllocator::getPoolUsagePercent();
}

size_t Engine::getPoolCapacity() {
    return GameObjectAllocator::getPoolCapacity();
}

void Engine::removeGameObject(GameObject* obj) {
    std::lock_guard<std::mutex> lock(s_gameObjectsMutex);
    auto it = std::find(s_gameObjects.begin(), s_gameObjects.end(), obj);
    if (it != s_gameObjects.end()) {
        GameObjectAllocator::destroy(*it); // Uses proper allocator
        s_gameObjects.erase(it);
    }
}

void Engine::queueRemove(GameObject* obj) {
	std::lock_guard<std::mutex> lock(s_gameObjectsMutex);
	s_pendingRemovals.push_back(obj);
}


std::vector<GameObject*> Engine::getGameObjectsSnapshot() {
    std::lock_guard<std::mutex> lock(s_gameObjectsMutex);
    return s_gameObjects;
}

std::mutex& Engine::getGameObjectsMutex() {
    return s_gameObjectsMutex;
}

SDL_Renderer* Engine::getRenderer() {
    return s_renderer;
}

void Engine::run(std::function<void(float)> update, std::function<void(void)> render) {
	s_running = true;
	s_workerRunning = true;

	// Start background update thread
	s_updateThread = std::thread([&]() {
		Uint64 last = SDL_GetTicks();
		while (s_workerRunning) {
			Uint64 now = SDL_GetTicks();
			float dt = (now - last) / 1000.0f;
			last = now;

			std::vector<GameObject*> snapshot;
			std::unordered_set<GameObject*> pendingSet;

			{
				std::lock_guard<std::mutex> lock(s_gameObjectsMutex);
				snapshot = s_gameObjects;

				pendingSet = std::unordered_set<GameObject*>(
					s_pendingRemovals.begin(),
					s_pendingRemovals.end()
				);
			}

			for (GameObject* obj : snapshot) {
				if (!obj) continue;

				// Skip if object is pending removal
				if (pendingSet.find(obj) != pendingSet.end()) continue;

				obj->update(dt);
			}

			SDL_Delay(1);
		}
		});

	// Main thread loop: events, input, update, render
	SDL_Event e;
	Uint64 lastTime = SDL_GetTicks();

	while (s_running) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_EVENT_QUIT) {
				s_running = false;
			}
		}

		Input::updateKeyboardState();

		Uint64 currentTime = SDL_GetTicks();
		float deltaTime = (currentTime - lastTime) / 1000.0f;
		lastTime = currentTime;

		update(deltaTime);

		// FLUSH REMOVALS BEFORE RENDERING SNAPSHOT
		{
			std::lock_guard<std::mutex> lock(s_gameObjectsMutex);
			for (GameObject* obj : s_pendingRemovals) {
				auto it = std::find(s_gameObjects.begin(), s_gameObjects.end(), obj);
				if (it != s_gameObjects.end()) {
					GameObjectAllocator::destroy(*it);
					s_gameObjects.erase(it);
				}
			}
			s_pendingRemovals.clear();
		}

		// Clear screen
		SDL_SetRenderDrawColor(s_renderer, 255, 255, 255, 255); // white background
		SDL_RenderClear(s_renderer);



		// Extra custom rendering (e.g. HUD)
		render();

		SDL_RenderPresent(s_renderer);
	}

	// Cleanup update thread
	s_workerRunning = false;
	if (s_updateThread.joinable()) s_updateThread.join();
}

