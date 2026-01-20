#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <unordered_set>

#include "bossActions.h"
#include "TagComponent.h"
#include "ProjectileComponent.h"
#include "HealthComponent.h"
#include "DashComponent.h"
#include "PlayerShootComponent.h"
#include "BossComponent.h"
#include "SinusoidalProjectileComponent.h"

#include <engine/Engine.h>
#include <engine/Input.h>
#include <engine/Timeline.h>
#include <engine/Collision.h>
#include <engine/GameObject.h>
#include <engine/TransformComponent.h>
#include <engine/RenderComponent.h>
#include <engine/ColliderComponent.h>
#include <engine/GravityComponent.h>
#include <engine/InputComponent.h>
#include <engine/Event.h>
#include <engine/EventManager.h>
#include <engine/GameObjectAllocator.hpp>
#include <engine/GameObjectPool.hpp>


// Setup for timeline hud and speeds
TTF_Font* hudFont = nullptr;
const std::vector<float> speedLevels = { 0.5f, 1.0f, 2.0f };
size_t currentSpeedIndex = 1;

// SDL texture for hud font
SDL_Texture* renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), text.length(), color);
    if (!surface) return nullptr;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    return texture;
}

// Global event manager
EventManager eventManager;
std::vector<Event> pendingEvents; // for deferred events to avoid deadlock

// Current player state setup, probably move to another class and add things like health
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

	// Shoot and dash state
	bool wantsToShoot = false;
	bool wantsToDashLeft = false;
	bool wantsToDashRight = false;
};
LocalPlayerState playerState;

// Global boss ptr
GameObject* globalBoss = nullptr;

// Sets up inputs from Actions.h
void setupInputBindings() {
    Input::clearBindings();
    Input::bindAction(SDL_SCANCODE_W, 0);      // Jump
    Input::bindAction(SDL_SCANCODE_S, 1);      // Dodge
    Input::bindAction(SDL_SCANCODE_UP, 2);     // Scale up
    Input::bindAction(SDL_SCANCODE_DOWN, 3);   // Scale down
    Input::bindAction(SDL_SCANCODE_SPACE, 4);  // Pause
    Input::bindAction(SDL_SCANCODE_A, 5);      // Left
    Input::bindAction(SDL_SCANCODE_D, 6);      // Right

	// Bind mouse button
	Input::bindMouseAction(SDL_BUTTON_LEFT, 7); // Shoot

	// Bind chords
	std::vector<SDL_Scancode> dashLeftChord = { SDL_SCANCODE_A, SDL_SCANCODE_S};
	Input::bindChord(dashLeftChord, 8); // dash left

	std::vector<SDL_Scancode> dashRightChord = { SDL_SCANCODE_D, SDL_SCANCODE_S };
	Input::bindChord(dashRightChord, 9); // dash right
}

// Maybe setup all event handlers here, maybe in a different class
void setupEventBindings(GameObject* player, Timeline& timeline) {
	
	// INPUT EVENT HANDLER
	eventManager.subscribe("InputPressed", [player](const Event& e) {
		int playerId = e.getParam("playerId").asInt;
		int actionIndex = e.getParam("key").asInt;  // This is the bit index

		// Set state based on which action was pressed
		if (actionIndex == 0) {  // Jump (W key)
			playerState.wantsToJump = true;
		}

		if (actionIndex == 1) {  // Dodge (S key)
			playerState.wantsToDodge = true;
		}

		if (actionIndex == 5) {  // Left (A key)
			playerState.movingLeft = true;
		}
		if (actionIndex == 6) {  // Right (D key)
			playerState.movingRight = true;
		}
		if (actionIndex == 7) {  // Shoot (left mouse)
			playerState.wantsToShoot = true;
		}
		if (actionIndex == 8) {  // Dash left (A + S)
			playerState.wantsToDashLeft = true;
		}
		if (actionIndex == 9) {  // Dash right (D +S)
			playerState.wantsToDashRight = true;
		}
	});

	// COLLISION EVENT HANDLER
	eventManager.subscribe("Collision", [](const Event& e) {
		GameObject* a = e.getParam("a").type == Variant::Type::GAMEOBJECT
			? e.getParam("a").asGameObject
			: nullptr;
		GameObject* b = e.getParam("b").type == Variant::Type::GAMEOBJECT
			? e.getParam("b").asGameObject
			: nullptr;
		if (!a || !b) return;

		auto* tA = a->getComponent<TransformComponent>();
		auto* tB = b->getComponent<TransformComponent>();
		if (!tA || !tB) return;

		auto* tagA = a->getComponent<TagComponent>();
		auto* tagB = b->getComponent<TagComponent>();
		if (!tagA || !tagB) return;

		std::string tag1 = tagA->getTag();
		std::string tag2 = tagB->getTag();

		// Projectile hit boss
		GameObject* projectile = nullptr;
		GameObject* boss = nullptr;

		if (tag1 == "projectile" && tag2 == "boss") {
			projectile = a; boss = b;
		}
		else if (tag2 == "projectile" && tag1 == "boss") {
			projectile = b; boss = a;
		}

		if (projectile && boss) {
			// Apply dmg to boss
			auto* bossHealth = boss->getComponent<HealthComponent>();
			auto* projComp = projectile->getComponent<ProjectileComponent>();

			if (bossHealth && projComp) {

				bossHealth->takeDamage(projComp->getDamage());
				projComp->onHit();

				std::cout << "Boss hit! Health: " << bossHealth->getCurrentHealth()
					<< "/" << bossHealth->getMaxHealth() << std::endl;

				// Check for boss death
				if (!bossHealth->isAlive()) {
					std::cout << "Boss Defeated!" << std::endl;

					Engine::queueRemove(boss);
					globalBoss = nullptr;
				}

			}
			return;
		}

		// Boss projectile hit player
		GameObject* bossProjectile = nullptr;
		GameObject* player = nullptr;

		if (tag1 == "boss_projectile" && tag2 == "player") {
			bossProjectile = a; player = b;
		}
		else if (tag2 == "boss_projectile" && tag1 == "player") {
			bossProjectile = b; player = a;
		}

		if (!playerState.dodgeActive && bossProjectile && player) {
			auto* playerHealth = player->getComponent<HealthComponent>();

			// Check which type of projectile hit
			auto* sinProj = bossProjectile->getComponent<SinusoidalProjectileComponent>();
			auto* regProj = bossProjectile->getComponent<ProjectileComponent>();

			int damage = 0;
			if (sinProj) {
				damage = sinProj->getDamage();
				sinProj->onHit();
			}
			else if (regProj) {
				damage = regProj->getDamage();
				regProj->onHit();
			}

			if (playerHealth && damage > 0) {
				playerHealth->takeDamage(damage);
				std::cout << "Player hit! Health: "
					<< playerHealth->getCurrentHealth() << "/"
					<< playerHealth->getMaxHealth() << std::endl;
			}

			return;
		}

		// Identify player and other object regardless of order
		GameObject* playerObj = nullptr;
		GameObject* otherObj = nullptr;
		std::string otherTag;

		if (tag1 == "player") { playerObj = a; otherObj = b; otherTag = tag2; }
		else if (tag2 == "player") { playerObj = b; otherObj = a; otherTag = tag1; }

		if (!playerObj || !otherObj) return;

		auto* playerT = playerObj->getComponent<TransformComponent>();
		auto* otherT = otherObj->getComponent<TransformComponent>();
		if (!playerT || !otherT) return;

		SDL_FRect playerRect = { playerT->getPosition().x, playerT->getPosition().y, playerT->getSize().x, playerT->getSize().y };
		SDL_FRect otherRect = { otherT->getPosition().x, otherT->getPosition().y, otherT->getSize().x, otherT->getSize().y };

		// === 1. WALL COLLISIONS ===
		if (otherTag == "wall") {
			// Player hit left side
			if (playerRect.x < otherRect.x && playerRect.x + playerRect.w > otherRect.x) {
				playerT->setPosition(otherRect.x - playerRect.w, playerRect.y);
			}
			// Player hit right side
			else if (playerRect.x + playerRect.w > otherRect.x && playerRect.x < otherRect.x + otherRect.w) {
				playerT->setPosition(otherRect.x + otherRect.w, playerRect.y);
			}
			// Stop horizontal movement
			Vec2 vel = playerT->getVelocity();
			vel.x = 0.f;
			playerT->setVelocity(vel.x, vel.y);
		}

		// === 2. PLATFORM COLLISIONS ===
		else if (otherTag == "platform") {
			// Check if player is landing from above
			if (playerRect.y + playerRect.h > otherRect.y && playerRect.y < otherRect.y) {
				playerT->setPosition(playerRect.x, otherRect.y - playerRect.h);
				Vec2 vel = playerT->getVelocity();
				vel.y = 0.f;
				playerT->setVelocity(vel.x, vel.y);
				playerState.isOnGround = true;
			}
		}
		});

	// Commented out death/spawn events and handled them manually

	// DEATH EVENT HANDLER
	// eventManager.subscribe("Death", [](const Event& e) {
	//	int playerId = e.getParam("playerId").asInt;

		// Create spawn event after death
	//	Event spawn("Spawn");
	//	spawn.addParam("playerId", Variant(playerId));
	//	spawn.addParam("spawnIndex", Variant(0)); // Can add logic for diff spawns later if needed
	//	spawn.priority = 1;

		// Defer it instead of raising immediately
	//	pendingEvents.push_back(spawn);

	//	});

	// SPAWN EVENT HANDLER
	// eventManager.subscribe("Spawn", [&player](const Event& e) {
	//	int playerId = e.getParam("playerId").asInt;
	//	int spawnIndex = e.getParam("spawnIndex").asInt;

		// Choose respawn location based on index
	//	float spawnX = 300.f, spawnY = 500.f;
	//	if (spawnIndex == 1) {
	//		spawnX = 700.f;
	//		spawnY = 400.f;
	//	}

		// Apply to player
	//	auto* t = player->getComponent<TransformComponent>();
	//	if (t) {
	//		t->setPosition(spawnX, spawnY);
	//		t->setVelocity(0.f, 0.f);
	//	}

	//	// Reset player health
	//	auto* health = player->getComponent<HealthComponent>();
	//	if (health) {
	//		health->heal(health->getMaxHealth());
	//	}

		// Reset state
	//	playerState.needsRespawn = false;
	//	playerState.isOnGround = false;
	//	});
}

void renderPoolHUD(SDL_Renderer* renderer, TTF_Font* font) {

	float usagePercent = GameObjectAllocator::getPoolUsagePercent();
	size_t capacity = GameObjectAllocator::getPoolCapacity();

	// Background bar
	SDL_FRect hudBackground = { 10, 50, 200, 30 };
	SDL_SetRenderDrawColor(renderer, 50, 50, 50, 200);
	SDL_RenderFillRect(renderer, &hudBackground);

	// Usage bar
	SDL_FRect hudBar = { 15, 55, (usagePercent / 100.0f) * 190.0f, 20 };

	// Color coding for usage
	if (usagePercent < 50.0f) {
		SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green
	}
	else if (usagePercent < 80.0f) {
		SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow
	}
	else {
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red
	}
	SDL_RenderFillRect(renderer, &hudBar);

	// Border
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderRect(renderer, &hudBackground);

	// Text label
	SDL_Color white = { 255, 255, 255, 255 };
	std::stringstream ss;
	ss << "Pool: " << usagePercent << "% (" << capacity << " objects)";
	SDL_Texture* tex = renderText(renderer, font, ss.str(), white);
	if (tex) {
		float w, h;
		SDL_GetTextureSize(tex, &w, &h);
		SDL_FRect dst = { 220, 55, w, h };
		SDL_RenderTexture(renderer, tex, NULL, &dst);
		SDL_DestroyTexture(tex);
	}
}


int main(int argc, char* argv[]) {

	// Temp playerID, was needed for networking
	int playerID = 0;

	// Window name and size
	Engine::Config config;
	config.title = "Boss Game";
	config.width = 1920;
	config.height = 1080;

	// Init hud font
	if (TTF_Init() < 0) {
		SDL_Log("Failed to init TTF: %s", SDL_GetError());
		return 1;
	}

	hudFont = TTF_OpenFont("assets/DejaVuSans.ttf", 24);
	if (!hudFont) {
		SDL_Log("Failed to load font: %s", SDL_GetError());
		return 1;
	}

	// Init engine
	if (!Engine::init(config)) {
		SDL_Log("Failed to initialize engine: %s", SDL_GetError());
		return 1;
	}

	// Init Pool Allocator
	GameObjectAllocator::setMode(GameObjectAllocator::POOLED);
	GameObjectAllocator::setPoolCapacity(20);

	SDL_Log("GameObject Pool initialized:");
	SDL_Log("  - Capacity: %zu objects", GameObjectAllocator::getPoolCapacity());
	SDL_Log("  - Mode: POOLED");

	// Call input setup function
	setupInputBindings();

	// Init timeline to default speed
	Timeline timeline;
	timeline.init();
	timeline.setScale(speedLevels[currentSpeedIndex]);

	// Test screen wide brick
	auto* brickGround = GameObjectAllocator::create();
	brickGround->addComponent(std::make_unique<TagComponent>("platform"));
	brickGround->addComponent(std::make_unique<TransformComponent>(0, 800, 1920, 32));
	brickGround->addComponent(std::make_unique<RenderComponent>("assets/Brick.png", true));
	brickGround->addComponent(std::make_unique<ColliderComponent>());
	Engine::addGameObject(brickGround);

	// Test invisible walls
	auto* leftWall = GameObjectAllocator::create();
	leftWall->addComponent(std::make_unique<TagComponent>("wall"));
	leftWall->addComponent(std::make_unique<TransformComponent>(0, 0, 40, 800));
	leftWall->addComponent(std::make_unique<ColliderComponent>());
	Engine::addGameObject(leftWall);

	auto* rightWall = GameObjectAllocator::create();
	rightWall->addComponent(std::make_unique<TagComponent>("wall"));
	rightWall->addComponent(std::make_unique<TransformComponent>(1920, 0, 40, 800));
	rightWall->addComponent(std::make_unique<ColliderComponent>());
	Engine::addGameObject(rightWall);

	// Spawn points
	auto* defaultSpawn = GameObjectAllocator::create();
	defaultSpawn->addComponent(std::make_unique<TagComponent>("spawn"));
	defaultSpawn->addComponent(std::make_unique<TransformComponent>(300, 500));
	Engine::addGameObject(defaultSpawn);

	// Player
	auto* player = GameObjectAllocator::create();
	player->addComponent(std::make_unique<TagComponent>("player"));
	player->addComponent(std::make_unique<TransformComponent>(playerState.respawnX, playerState.respawnY, 64, 64));
	player->addComponent(std::make_unique<RenderComponent>("assets/Morwen.png"));
	player->addComponent(std::make_unique<GravityComponent>(300.f));
	player->addComponent(std::make_unique<InputComponent>());
	player->addComponent(std::make_unique<ColliderComponent>());
	player->addComponent(std::make_unique<DashComponent>());
	player->addComponent(std::make_unique<PlayerShootComponent>());
	player->addComponent(std::make_unique<HealthComponent>(100));
	Engine::addGameObject(player);

	// Boss
	auto* boss = GameObjectAllocator::create();
	boss->addComponent(std::make_unique<TagComponent>("boss"));
	boss->addComponent(std::make_unique<TransformComponent>(1600, 200, 256, 256));
	boss->addComponent(std::make_unique<RenderComponent>("assets/boss.png"));
	boss->addComponent(std::make_unique<ColliderComponent>());
	boss->addComponent(std::make_unique<HealthComponent>(500));
	boss->addComponent(std::make_unique<BossComponent>(player, 150.0f, 1400.0f, 1800.0f));
	Engine::addGameObject(boss);

	globalBoss = boss; // update our global ptr

	// Pointer to check for platform collisions
	GameObject* lastPlatform = nullptr;

	// Initial timeline vals
	int currentTick = 0;
	bool wasScaleUp = false, wasScaleDown = false, wasPause = false;

	// Track if we need to respawn (set by collision, applied safely later)
	bool needsRespawn = false;

	// Setup event handlers for player after creating player object
	setupEventBindings(player, timeline);

	// Game loop
	Engine::run(
		[&](float rawDelta) {

			// Get total time
			float now = static_cast<float>(timeline.getAccumulatedTime());

			// Get player input
			auto* inputComp = player->getComponent<InputComponent>();
			if (!inputComp) return;
			uint32_t actionMask = inputComp->getActionMask();

			// STEP: Reset movement state each frame
			playerState.movingLeft = false;
			playerState.movingRight = false;
			playerState.wantsToJump = false;
			playerState.wantsToDodge = false;

			playerState.wantsToShoot = false;
			playerState.wantsToDashLeft = false;
			playerState.wantsToDashRight = false;

			// STEP: Raise input events for all pressed keys
			for (int i = 0; i < 32; ++i) {
				if (actionMask & (1 << i)) {
					Event inputEvent("InputPressed");
					inputEvent.addParam("playerId", Variant(playerID));
					inputEvent.addParam("key", Variant(i));
					inputEvent.priority = 0;
					eventManager.raise(inputEvent, now);
				}
			}

			// STEP: Collision check for all GameObjects before dispatch
			auto snapshot = Engine::getGameObjectsSnapshot(); // const snapshot

			std::unordered_set<GameObject*> processedRemovals;  // Track objects already marked


			for (GameObject* objA : snapshot) {

				// Skip if already marked for removal
				if (processedRemovals.find(objA) != processedRemovals.end()) continue;

				auto* colA = objA->getComponent<ColliderComponent>();
				auto* tA = objA->getComponent<TransformComponent>();
				if (!colA || !tA || !colA->isCollidable()) continue;

				for (GameObject* objB : snapshot) {
					if (objA == objB) continue;

					// Skip if already marked for removal
					if (processedRemovals.find(objB) != processedRemovals.end()) continue;

					auto* colB = objB->getComponent<ColliderComponent>();
					auto* tB = objB->getComponent<TransformComponent>();
					if (!colB || !tB || !colB->isCollidable()) continue;

					if (Collision::checkCollision(*objA, *objB)) {
						Event e("Collision");
						e.addParam("a", Variant(objA));
						e.addParam("b", Variant(objB));
						eventManager.raise(e, now);

						// Mark projectiles as processed if they might be removed
						auto* tagA = objA->getComponent<TagComponent>();
						auto* tagB = objB->getComponent<TagComponent>();
						if (tagA && (tagA->getTag() == "projectile" || tagA->getTag() == "boss_projectile")) {
							processedRemovals.insert(objA);
						}
						if (tagB && (tagB->getTag() == "projectile" || tagB->getTag() == "boss_projectile")) {
							processedRemovals.insert(objB);
						}
					}
				}
			}

			// STEP: Dispatch events (updates playerState)
			eventManager.dispatch(now);

			// Handle events raised by other event handlers
			for (auto& ev : pendingEvents)
				eventManager.raise(ev, timeline.getAccumulatedTime());
			pendingEvents.clear();

			// Dispatch deferred events
			eventManager.dispatch(now);

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
				auto* transform = player->getComponent<TransformComponent>();
				if (transform) transform->setVelocity(0.f, 0.f);
				return;
			}

			// STEP: Check for death and respawn directly
			{
				auto* t = player->getComponent<TransformComponent>();
				auto* health = player->getComponent<HealthComponent>();

				if (t && health) {

					bool needsRespawn = false;

					// Check if fell off map
					if (t->getPosition().y > 1080.0f) {
						std::cout << "Player fell off map" << std::endl;
						needsRespawn = true;
					}

					// Check if health ran out
					if (!health->isAlive()) {
						std::cout << "Player health depleted" << std::endl;
						needsRespawn = true;
					}

					// Respawn immediately
					if (needsRespawn) {
						std::cout << "Respawned" << std::endl;

						// Reset position
						t->setPosition(playerState.respawnX, playerState.respawnY);
						t->setVelocity(0.f, 0.f);

						// Reset health
						health->revive();

						// Reset state
						playerState.isOnGround = false;
						playerState.dodgeActive = false;
						playerState.dodgeTimer = 0.0f;

					}
				}
			}

			// STEP: Movement & Physics
			auto* transform = player->getComponent<TransformComponent>();
			auto* gravity = player->getComponent<GravityComponent>();
			if (!transform) return;

			Vec2 pos = transform->getPosition();
			Vec2 vel = transform->getVelocity();

			float moveSpeed = 300.f;
			float jumpForce = -300.f;


			// STEP: Handle dash before movement
			auto* dashComp = player->getComponent<DashComponent>();

			if (dashComp && transform) {
				if (playerState.wantsToDashLeft) {
					dashComp->tryStartDash(transform, -1.0f);
				}
				else if (playerState.wantsToDashRight) {
					dashComp->tryStartDash(transform, 1.0f);
				}
			}

			bool isDashing = dashComp && dashComp->isCurrentlyDashing();

			// STEP: Apply gravity if not dashing
			if (gravity && !isDashing) {
				gravity->update(*player, scaledDelta);
				vel = transform->getVelocity();
			}

			// STEP: Apply movement if not dashing
			if (!isDashing) {
				if (playerState.movingLeft)
					vel.x = -moveSpeed;
				else if (playerState.movingRight)
					vel.x = moveSpeed;
				else
					vel.x = 0.f;
			}

			// STEP: Handle dodge
			if (playerState.wantsToDodge && !playerState.dodgeActive) {
				playerState.dodgeActive = true;
				playerState.dodgeTimer = 1.5f;
			}
			if (playerState.dodgeActive) {
				playerState.dodgeTimer -= scaledDelta;
				if (playerState.dodgeTimer <= 0.f)
					playerState.dodgeActive = false;
			}

			// STEP: Get final velocity 
			if (isDashing) {
				vel = transform->getVelocity();  // Get dash velocity from DashComponent
			}

			// Update position
			pos.x += vel.x * scaledDelta;
			pos.y += vel.y * scaledDelta;
			transform->setPosition(pos.x, pos.y);

			if (!isDashing) {
				transform->setVelocity(vel.x, vel.y);
			}

			// STEP: Handle shooting
			auto* shootComp = player->getComponent<PlayerShootComponent>();
			if (shootComp && transform && playerState.wantsToShoot) {
				float mouseX, mouseY;
				Input::getMousePosition(mouseX, mouseY);
				shootComp->tryShoot(transform, mouseX, mouseY);
			}

			// STEP: Handle jump (after ground check)
			if (playerState.wantsToJump && playerState.isOnGround) {
				vel.y = jumpForce;
				transform->setVelocity(vel.x, vel.y);
			}

		},

		[&]() {

			// Init renderer
			SDL_Renderer* renderer = Engine::getRenderer();

			// Render every game object in the current snapshot
			std::vector<GameObject*> objs = Engine::getGameObjectsSnapshot();
			for (auto* obj : objs) {
				if (!obj) continue;
				auto* renderComp = obj->getComponent<RenderComponent>();
				if (!renderComp) continue;
				renderComp->draw(*obj, renderer);
			}

			// Timeline hud
			SDL_Color black = { 0,0,0,255 };
			std::stringstream ss;
			ss << "Speed: x" << timeline.getScale();
			if (timeline.isPaused()) ss << " [PAUSED]";
			SDL_Texture* tex = renderText(renderer, hudFont, ss.str(), black);
			if (tex) {
				float w, h; SDL_GetTextureSize(tex, &w, &h);
				SDL_FRect dst = { 10, 10, w, h };
				SDL_RenderTexture(renderer, tex, NULL, &dst);
				SDL_DestroyTexture(tex);
			}

			renderPoolHUD(renderer, hudFont);

			// Render health HUD
			if (player) {
				auto* playerHealth = player->getComponent<HealthComponent>();
				if (playerHealth) {
					SDL_Color healthColor = { 0, 255, 0, 255 };  // Green

					std::stringstream healthSS;
					healthSS << "Player HP: " << playerHealth->getCurrentHealth()
						<< "/" << playerHealth->getMaxHealth();
					SDL_Texture* healthTex = renderText(renderer, hudFont, healthSS.str(), healthColor);
					if (healthTex) {
						float w, h;
						SDL_GetTextureSize(healthTex, &w, &h);
						SDL_FRect healthDst = { 10, 90, w, h };
						SDL_RenderTexture(renderer, healthTex, NULL, &healthDst);
						SDL_DestroyTexture(healthTex);
					}
				}
			}

			// Render boss health HUD
			if (globalBoss) {
				auto* bossHealth = globalBoss->getComponent<HealthComponent>();
				if (bossHealth) {
					SDL_Color bossHealthColor = { 255, 0, 0, 255 }; // Red

					std::stringstream bossSS;
					bossSS << "Boss HP: " << bossHealth->getCurrentHealth()
						<< "/" << bossHealth->getMaxHealth();
					SDL_Texture* bossTex = renderText(renderer, hudFont, bossSS.str(), bossHealthColor);
					if (bossTex) {
						float w, h;
						SDL_GetTextureSize(bossTex, &w, &h);
						SDL_FRect bossDst = { 10, 120, w, h };
						SDL_RenderTexture(renderer, bossTex, NULL, &bossDst);
						SDL_DestroyTexture(bossTex);
					}
				}
			}
		}
	);

	// Clean up
	Engine::shutdown();

	if (hudFont) TTF_CloseFont(hudFont);
	TTF_Quit;

}