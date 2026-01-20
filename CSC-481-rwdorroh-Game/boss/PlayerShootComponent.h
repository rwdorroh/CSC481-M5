#pragma once
#include <engine/Component.h>
#include <engine/TransformComponent.h>
#include <engine/InputComponent.h>
#include <engine/RenderComponent.h>
#include <engine/ColliderComponent.h>
#include <engine/Engine.h>
#include <engine/Input.h>
#include <engine/GameObjectAllocator.hpp>

#include "bossActions.h"
#include "TagComponent.h"
#include "ProjectileComponent.h"
#include <cmath>
#include <memory>

class PlayerShootComponent : public Component {

public:
	
	// Shooting speed
	float shootCooldown = 0.0f;
	const float SHOOT_RATE = 0.25f;

	// Projectile Params
	const float PROJECTILE_SPEED = 600.0f;
	const float PROJECTILE_LIFETIME = 3.0f;
	const int PROJECTILE_DAMAGE = 10;

	void update(GameObject& obj, float dt) override {

		// Update cooldown
		if (shootCooldown > 0) {
			shootCooldown -= dt;
		}
	}

	void tryShoot(TransformComponent* transform, float mouseX, float mouseY) {

		if (shootCooldown > 0) return;
		if (!transform) return;

		Vec2 pos = transform->getPosition();
		Vec2 size = transform->getSize();

		float startX = pos.x + size.x / 2.0f;
		float startY = pos.y + size.y / 2.0f;

		float dx = mouseX - startX;
		float dy = mouseY - startY;
		float length = std::sqrt(dx * dx + dy * dy);

		if (length > 0) {
			dx /= length;
			dy /= length;

			fireProjectile(startX, startY, dx, dy);
			shootCooldown = SHOOT_RATE;
		}

	}

	bool canShoot() const { return shootCooldown <= 0; }

private:

	void fireProjectile(float x, float y, float dirX, float dirY) {

		// CHECK POOL CAPACITY BEFORE SPAWNING
		float usagePercent = GameObjectAllocator::getPoolUsagePercent();
		if (usagePercent >= 95.0f) {
			SDL_Log("WARNING: Pool nearly full (%.1f%%), cannot shoot!", usagePercent);
			return;  // Don't spawn if pool is almost full
		}

		// Try to create projectile
		GameObject* projectile = GameObjectAllocator::create();
		if (!projectile) {
			SDL_Log("ERROR: Failed to spawn projectile - pool is FULL!");
			return;
		}

		projectile->addComponent(std::make_unique<TagComponent>("projectile"));
		projectile->addComponent(std::make_unique<TransformComponent>(x - 5.0f, y - 5.0f, 32.0f, 32.0f, dirX * PROJECTILE_SPEED, dirY * PROJECTILE_SPEED));
		projectile->addComponent(std::make_unique<ProjectileComponent>(PROJECTILE_LIFETIME, PROJECTILE_DAMAGE));
		projectile->addComponent(std::make_unique<ColliderComponent>());
		projectile->addComponent(std::make_unique<RenderComponent>("assets/lanternShot.png"));

		Engine::addGameObject(projectile);
	}

};