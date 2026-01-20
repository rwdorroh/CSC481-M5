#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <thread>
#include <mutex>

#include "Actions.h"
#include "CameraComponent.h"

#include <engine/Engine.h>
#include <engine/Client.h>
#include <engine/Input.h>
#include <engine/Timeline.h>
#include <engine/NetworkTypes.h>
#include <engine/Collision.h>
#include <engine/GameObject.h>
#include <engine/TransformComponent.h>
#include <engine/RenderComponent.h>
#include <engine/ColliderComponent.h>
#include <engine/GravityComponent.h>
#include <engine/InputComponent.h>
#include <engine/NetworkComponent.h>
#include <engine/Event.h>
#include <engine/EventManager.h>

TTF_Font* hudFont = nullptr;
const std::vector<float> speedLevels = { 0.5f, 1.0f, 2.0f };
size_t currentSpeedIndex = 1;

SDL_Texture* renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), text.length(), color);
    if (!surface) return nullptr;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    return texture;
}

std::mutex stateMutex;
struct ServerSnapshot {
    std::unordered_map<int, Vec2> otherPlayersPositions;
    std::vector<SyncedObjectData> syncedObjects;
    bool valid = false;
    std::vector<int> removedIds;
};
ServerSnapshot latestSnapshot;

EventManager GlobalEventManager;

struct LocalPlayerState {
	bool isOnGround = false;
	bool dodgeActive = false;
	float dodgeTimer = 0.0f;

	bool needsRespawn = false;
	float respawnX = 300.f;
	float respawnY = 500.f;

	// Movement state
	bool movingLeft = false;
	bool movingRight = false;
	bool wantsToJump = false;
	bool wantsToDodge = false;
};

LocalPlayerState playerState;

void setupInputBindings() {
    Input::clearBindings();
    Input::bindAction(SDL_SCANCODE_W, 0);      // Jump
    Input::bindAction(SDL_SCANCODE_S, 1);      // Dodge
    Input::bindAction(SDL_SCANCODE_UP, 2);     // Scale up
    Input::bindAction(SDL_SCANCODE_DOWN, 3);   // Scale down
    Input::bindAction(SDL_SCANCODE_SPACE, 4);  // Pause
    Input::bindAction(SDL_SCANCODE_A, 5);      // Left
    Input::bindAction(SDL_SCANCODE_D, 6);      // Right
}

void setupEventBindings(GameObject* localPlayer) {
	GlobalEventManager.subscribe("InputPressed", [localPlayer](const Event& e) {
		int playerId = e.getParam("playerId").asInt;
		int actionIndex = e.getParam("key").asInt;  // This is the bit index (0-6)

		// Set state based on which action was pressed
		if (actionIndex == 5) {  // Left (A key)
			playerState.movingLeft = true;
		}
		else if (actionIndex == 6) {  // Right (D key)
			playerState.movingRight = true;
		}

		if (actionIndex == 0) {  // Jump (W key)
			playerState.wantsToJump = true;
		}

		if (actionIndex == 1) {  // Dodge (S key)
			playerState.wantsToDodge = true;
		}
		});

	GlobalEventManager.subscribe("Collision", [](const Event& e) {
		int playerId = e.getParam("playerId").asInt;
		int objectId = e.getParam("objectId").asInt;
		std::cout << "[Collision] Player " << playerId << " collided with object " << objectId << "\n";
		});
}

void networkReceiveThread(Client& net, int playerID) {
    while (true) {
        WorldSnapshot snapshot;
        if (!net.pollUpdate(snapshot)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        std::lock_guard<std::mutex> lock(stateMutex);
        latestSnapshot.syncedObjects = snapshot.syncedObjects;

        // Append new removed IDs (don't overwrite until main thread processes them)
        for (int id : snapshot.removedIds) {
            if (std::find(latestSnapshot.removedIds.begin(), latestSnapshot.removedIds.end(), id) == latestSnapshot.removedIds.end()) {
                latestSnapshot.removedIds.push_back(id);
            }
        }

        latestSnapshot.otherPlayersPositions.clear();

        for (size_t i = 0; i < snapshot.playerIds.size(); ++i) {
            if (snapshot.playerIds[i] != playerID) {
                latestSnapshot.otherPlayersPositions[snapshot.playerIds[i]] = snapshot.playerPositions[i];
            }
        }
        latestSnapshot.valid = true;
    }
}

int main(int argc, char* argv[]) {
    int playerID;
    std::cout << "Enter player ID (integer): ";
    std::cin >> playerID;

    Client::setClientID(playerID);

    Engine::Config config;
    config.title = "CSC 481 Game";
    config.width = 1900;
    config.height = 1000;

    if (TTF_Init() < 0) {
        SDL_Log("Failed to init TTF: %s", SDL_GetError());
        return 1;
    }

    hudFont = TTF_OpenFont("assets/DejaVuSans.ttf", 24);
    if (!hudFont) {
        SDL_Log("Failed to load font: %s", SDL_GetError());
        return 1;
    }

    if (!Engine::init(config)) {
        SDL_Log("Failed to initialize engine: %s", SDL_GetError());
        return 1;
    }

    setupInputBindings();

    Timeline timeline;
    timeline.init();
    timeline.setScale(speedLevels[currentSpeedIndex]);

    Client net;
    bool isConnected = net.connect();

    // Static platforms
    auto* platform = new GameObject();
    platform->addComponent(std::make_unique<TransformComponent>(300, 800, 96, 32));
    platform->addComponent(std::make_unique<RenderComponent>("assets/Brick.png"));
    platform->addComponent(std::make_unique<ColliderComponent>());
    Engine::addGameObject(platform);

    auto* abovePlatform = new GameObject();
    abovePlatform->addComponent(std::make_unique<TransformComponent>(300, 400, 96, 32));
    abovePlatform->addComponent(std::make_unique<RenderComponent>("assets/Brick.png"));
    abovePlatform->addComponent(std::make_unique<ColliderComponent>());
    Engine::addGameObject(abovePlatform);

    auto* midPlatform = new GameObject();
    midPlatform->addComponent(std::make_unique<TransformComponent>(700, 600, 160, 32));
    midPlatform->addComponent(std::make_unique<RenderComponent>("assets/Brick.png"));
    midPlatform->addComponent(std::make_unique<ColliderComponent>());
    Engine::addGameObject(midPlatform);

    auto* movingPlatform = new GameObject();
    movingPlatform->addComponent(std::make_unique<TransformComponent>(1100.f, 800.f, 200.f, 32.f, 150.f, 0.f));
    movingPlatform->addComponent(std::make_unique<RenderComponent>("assets/Brick.png"));
    movingPlatform->addComponent(std::make_unique<ColliderComponent>());
    Engine::addGameObject(movingPlatform);

    // Spawn points
    auto* defaultSpawn = new GameObject();
    defaultSpawn->addComponent(std::make_unique<TransformComponent>(300, 500));
    Engine::addGameObject(defaultSpawn);
    auto* spawnTransform = defaultSpawn->getComponent<TransformComponent>();
    float spawnX = spawnTransform ? spawnTransform->getPosition().x : 300.f;
    float spawnY = spawnTransform ? spawnTransform->getPosition().y : 500.f;

    auto* mapDeathSpawn = new GameObject();
    mapDeathSpawn->addComponent(std::make_unique<TransformComponent>(700, 400));
    Engine::addGameObject(mapDeathSpawn);
    auto* deathSpawnTransform = mapDeathSpawn->getComponent<TransformComponent>();
    float deathSpawnX = deathSpawnTransform ? deathSpawnTransform->getPosition().x : 700.f;
    float deathSpawnY = deathSpawnTransform ? deathSpawnTransform->getPosition().y : 400.f;

    // Local player
    auto* localPlayer = new GameObject();
    localPlayer->addComponent(std::make_unique<TransformComponent>(spawnX, spawnY, 64, 64));
    localPlayer->addComponent(std::make_unique<RenderComponent>("assets/Morwen.png"));
    localPlayer->addComponent(std::make_unique<GravityComponent>(200.f));
    localPlayer->addComponent(std::make_unique<InputComponent>());
    localPlayer->addComponent(std::make_unique<ColliderComponent>());
    Engine::addGameObject(localPlayer);

    // Camera
    auto* camera = new GameObject();
    camera->addComponent(std::make_unique<CameraComponent>(config.width, config.height));
    Engine::addGameObject(camera);
    auto* camComp = camera->getComponent<CameraComponent>();
    if (camComp) camComp->setTarget(localPlayer);

    std::unordered_map<int, GameObject*> otherPlayers;
    GameObject* orb = nullptr;
    bool hasActivatedServerControl = false;
    GameObject* lastPlatform = nullptr;

    std::thread netThread;
    if (isConnected)
        netThread = std::thread(networkReceiveThread, std::ref(net), playerID);

    int currentTick = 0;
    bool wasScaleUp = false, wasScaleDown = false, wasPause = false;

    // Track if we need to respawn (set by collision, applied safely later)
    bool needsRespawn = false;
    float respawnX = spawnX;
    float respawnY = spawnY;

	setupEventBindings(localPlayer);

	Engine::run(
		[&](float rawDelta) {
			float now = static_cast<float>(timeline.getAccumulatedTime());

			auto* inputComp = localPlayer->getComponent<InputComponent>();
			if (!inputComp) return;

			uint32_t actionMask = inputComp->getActionMask();

			// STEP 1: Reset movement state each frame
			playerState.movingLeft = false;
			playerState.movingRight = false;
			playerState.wantsToJump = false;
			playerState.wantsToDodge = false;

			// STEP 2: Raise input events for all pressed keys
			for (int i = 0; i < 32; ++i) {
				if (actionMask & (1 << i)) {
					Event inputEvent("InputPressed");
					inputEvent.addParam("playerId", Variant(playerID));
					inputEvent.addParam("key", Variant(i));
					inputEvent.priority = 0;
					GlobalEventManager.raise(inputEvent, now);
				}
			}

			// STEP 3: Dispatch events (updates playerState)
			GlobalEventManager.dispatch(now);

			std::vector<Event> raisedEvents;

			// Timeline controls (keep as is)
			bool scaleUp = (actionMask & (1 << 2));
			if (scaleUp && !wasScaleUp && currentSpeedIndex < speedLevels.size() - 1) {
				timeline.setScale(speedLevels[++currentSpeedIndex]);
			}
			wasScaleUp = scaleUp;

			bool scaleDown = (actionMask & (1 << 3));
			if (scaleDown && !wasScaleDown && currentSpeedIndex > 0) {
				timeline.setScale(speedLevels[--currentSpeedIndex]);
			}
			wasScaleDown = scaleDown;

			bool pause = (actionMask & (1 << 4));
			if (pause && !wasPause) {
				timeline.isPaused() ? timeline.resume() : timeline.pause();
			}
			wasPause = pause;

			float scaledDelta = static_cast<float>(timeline.update());
			currentTick++;

			if (timeline.isPaused()) {
				auto* transform = localPlayer->getComponent<TransformComponent>();
				if (transform) transform->setVelocity(0.f, 0.f);
				return;
			}

			// STEP 4: Apply respawn if needed
			if (playerState.needsRespawn) {
				auto* transform = localPlayer->getComponent<TransformComponent>();
				if (transform) {
					transform->setPosition(playerState.respawnX, playerState.respawnY);
					transform->setVelocity(0.f, 0.f);
				}
				playerState.needsRespawn = false;
			}

			// STEP 5: Movement & Physics
			auto* transform = localPlayer->getComponent<TransformComponent>();
			auto* gravity = localPlayer->getComponent<GravityComponent>();
			if (!transform) return;

			Vec2 pos = transform->getPosition();
			Vec2 vel = transform->getVelocity();

			float moveSpeed = 300.f;
			float jumpForce = -500.f;

			if (gravity) {
				gravity->update(*localPlayer, scaledDelta);
				vel = transform->getVelocity();
			}

			// STEP 6: Apply movement based on event-driven state
			if (playerState.movingLeft)
				vel.x = -moveSpeed;
			else if (playerState.movingRight)
				vel.x = moveSpeed;
			else
				vel.x = 0.f;

			// STEP 7: Handle dodge
			if (playerState.wantsToDodge && !playerState.dodgeActive) {
				playerState.dodgeActive = true;
				playerState.dodgeTimer = 1.5f;
			}
			if (playerState.dodgeActive) {
				playerState.dodgeTimer -= scaledDelta;
				if (playerState.dodgeTimer <= 0.f)
					playerState.dodgeActive = false;
			}

			pos.x += vel.x * scaledDelta;
			pos.y += vel.y * scaledDelta;
			transform->setPosition(pos.x, pos.y);
			transform->setVelocity(vel.x, vel.y);

			// STEP 8: Platform collision
			bool isOnGround = false;
			GameObject* currentPlatform = nullptr;

			for (GameObject* obj : Engine::getGameObjectsSnapshot()) {
				if (obj == localPlayer) continue;

				auto* ot = obj->getComponent<TransformComponent>();
				auto* oc = obj->getComponent<ColliderComponent>();
				if (!ot || !oc || !oc->isCollidable()) continue;

				if (Collision::checkCollision(*localPlayer, *obj)) {
					SDL_FRect playerRect = { pos.x, pos.y, transform->getSize().x, transform->getSize().y };
					SDL_FRect otherRect = { ot->getPosition().x, ot->getPosition().y, ot->getSize().x, ot->getSize().y };

					if (playerRect.y + playerRect.h <= otherRect.y + 10.f) {
						pos.y = otherRect.y - playerRect.h;
						vel.y = 0.f;
						transform->setPosition(pos.x, pos.y);
						transform->setVelocity(vel.x, vel.y);
						isOnGround = true;
						currentPlatform = obj;
					}
				}
			}

			playerState.isOnGround = isOnGround;

			if (currentPlatform) {
				auto* platformTransform = currentPlatform->getComponent<TransformComponent>();
				if (platformTransform) {
					Vec2 platformVel = platformTransform->getVelocity();
					pos.x += platformVel.x * scaledDelta;
					pos.y += platformVel.y * scaledDelta;
					transform->setPosition(pos.x, pos.y);
				}
			}

			lastPlatform = currentPlatform;

			// STEP 9: Handle jump (after ground check)
			if (playerState.wantsToJump && playerState.isOnGround) {
				vel.y = jumpForce;
				transform->setVelocity(vel.x, vel.y);
			}

			// STEP 10: Orb collision
			if (orb && !playerState.dodgeActive) {
				if (Collision::checkCollision(*localPlayer, *orb)) {
					Event collision("Collision");
					collision.addParam("playerId", Variant(playerID));
					collision.addParam("objectId", Variant(1));
					collision.priority = 1;
					raisedEvents.push_back(collision);

					Event death("Death");
					death.addParam("playerId", Variant(playerID));
					death.priority = 2;
					raisedEvents.push_back(death);

					playerState.needsRespawn = true;
					playerState.respawnX = spawnX;
					playerState.respawnY = spawnY;
				}
			}

			// STEP 11: Death zone check
			{
				auto* t = localPlayer->getComponent<TransformComponent>();
				if (t && t->getPosition().y > 1080.f) {
					Event death("Death");
					death.addParam("playerId", Variant(playerID));
					raisedEvents.push_back(death);

					playerState.needsRespawn = true;
					playerState.respawnX = deathSpawnX;
					playerState.respawnY = deathSpawnY;
				}
			}

            // Update server-controlled objects FIRST (before collision checks)
            {
                std::lock_guard<std::mutex> lock(stateMutex);
                if (isConnected && latestSnapshot.valid) {
                    for (const auto& obj : latestSnapshot.syncedObjects) {
                        if (obj.id == 0 && obj.type == 0) {  // Moving platform
                            if (!movingPlatform) {
                                movingPlatform = new GameObject();
                                movingPlatform->addComponent(std::make_unique<TransformComponent>(obj.position.x, obj.position.y, 200, 32));
                                movingPlatform->addComponent(std::make_unique<RenderComponent>("assets/Brick.png"));
                                movingPlatform->addComponent(std::make_unique<ColliderComponent>());
                                movingPlatform->addComponent(std::make_unique<NetworkComponent>(false, obj.id, obj.type));
                                Engine::addGameObject(movingPlatform);
                            }
                            auto* t = movingPlatform->getComponent<TransformComponent>();
                            if (t) t->setPosition(obj.position.x, obj.position.y);
                        }

                        if (obj.id == 1 && obj.type == 1) {  // Orb
                            if (!orb) {
                                orb = new GameObject();
                                orb->addComponent(std::make_unique<TransformComponent>(1920 - 128, 0, 128, 128, -400, 180));
                                orb->addComponent(std::make_unique<RenderComponent>("assets/Orb.png"));
                                orb->addComponent(std::make_unique<ColliderComponent>());
                                orb->addComponent(std::make_unique<NetworkComponent>(false, obj.id, obj.type));
                                Engine::addGameObject(orb);
                            }

                            if (!hasActivatedServerControl && net.hasReceivedSnapshot()) {
                                auto* netComp = orb->getComponent<NetworkComponent>();
                                if (netComp && !netComp->serverControlled) {
                                    netComp->serverControlled = true;
                                    hasActivatedServerControl = true;
                                }
                            }

                            auto* t = orb->getComponent<TransformComponent>();
                            if (t) t->setPosition(obj.position.x, obj.position.y);
                        }
                    }

                    // Update other players' positions
                    for (auto& [id, pos] : latestSnapshot.otherPlayersPositions) {
                        if (otherPlayers.find(id) == otherPlayers.end()) {
                            auto* np = new GameObject();
                            np->addComponent(std::make_unique<TransformComponent>(pos.x, pos.y, 64, 64));
                            np->addComponent(std::make_unique<RenderComponent>("assets/Morwen.png"));
                            otherPlayers[id] = np;
                            Engine::addGameObject(np);
                        }
                        else {
                            auto* t = otherPlayers[id]->getComponent<TransformComponent>();
                            if (t) t->setPosition(pos.x, pos.y);
                        }
                    }

                    // Remove disconnected players (hold lock longer for thread safety)
                    if (!latestSnapshot.removedIds.empty()) {
                        std::cout << "[Client] Processing " << latestSnapshot.removedIds.size() << " removed player(s)\n";
                        for (int id : latestSnapshot.removedIds) {
                            auto it = otherPlayers.find(id);
                            if (it != otherPlayers.end()) {
                                GameObject* obj = it->second;
                                Engine::queueRemove(obj);
                                otherPlayers.erase(it);
                                std::cout << "[Client] Removed player " << id << " (disconnected)\n";
                            }
                            else {
                                std::cout << "[Client] Player " << id << " already removed\n";
                            }
                        }
                        latestSnapshot.removedIds.clear();
                    }
                }
            } // Release lock here

            // Network send
            if (isConnected) {
                auto* t = localPlayer->getComponent<TransformComponent>();
                if (t) {
                    ClientCommand cmd{ playerID, currentTick, actionMask, t->getPosition().x, t->getPosition().y };
                    cmd.events = raisedEvents;
                    net.sendCommand(cmd);
                }
            }
        },
        [&]() {
            SDL_Renderer* renderer = Engine::getRenderer();

            Vec2 cameraOffset{ 0.f, 0.f };
            auto* camComp = camera->getComponent<CameraComponent>();
            if (camComp) cameraOffset = camComp->getOffset();

            std::vector<GameObject*> objs = Engine::getGameObjectsSnapshot();
            for (auto* obj : objs) {
                if (!obj) continue;
                auto* renderComp = obj->getComponent<RenderComponent>();
                if (!renderComp) continue;
                renderComp->draw(*obj, renderer, cameraOffset);
            }

            SDL_Color black = { 0,0,0,255 };
            std::stringstream ss;
            ss << "Client ID: " << playerID << " | Speed: x" << timeline.getScale();
            if (timeline.isPaused()) ss << " [PAUSED]";
            SDL_Texture* tex = renderText(renderer, hudFont, ss.str(), black);
            if (tex) {
                float w, h; SDL_GetTextureSize(tex, &w, &h);
                SDL_FRect dst = { 10, 10, w, h };
                SDL_RenderTexture(renderer, tex, NULL, &dst);
                SDL_DestroyTexture(tex);
            }
        }
    );

    if (isConnected && netThread.joinable()) netThread.detach();

    if (isConnected) {
        net.disconnect();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    Engine::shutdown();

    if (hudFont) TTF_CloseFont(hudFont);
    TTF_Quit();

    return 0;
}