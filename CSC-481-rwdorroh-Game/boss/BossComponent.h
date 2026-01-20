#pragma once
#include <engine/Component.h>
#include <engine/TransformComponent.h>
#include <engine/ColliderComponent.h>
#include <engine/RenderComponent.h>
#include <engine/Engine.h>
#include <engine/GameObject.h>
#include <engine/GameObjectAllocator.hpp>

#include "TagComponent.h"
#include "ProjectileComponent.h"
#include "SinusoidalProjectileComponent.h"
#include <cmath>
#include <memory>

class BossComponent : public Component {
	
public: 

	// Movement
	float movementSpeed = 150.0f;
	float leftBound = 1400.0f;
	float rightBound = 1800.0f;
	float direction = -1.0f; // -1 or 1

	// Light Attack
	float lightAttackCooldown = 0.0f;
	const float LIGHT_ATTACK_RATE = 1.0f;
	const float LIGHT_PROJECTILE_SPEED = 400.0f;
	const int LIGHT_PROJECTILE_DAMAGE = 10;
	
	// Heavy Attack
	float heavyAttackCooldown = 0.0f;
	const float HEAVY_ATTACK_RATE = 10.0f;
	const float HEAVY_PROJECTILE_SPEED = 200.0f;
	const int HEAVY_PROJECTILE_DAMAGE = 40;
	const float WAVE_AMPLITUDE = 150.0f;
	const float WAVE_FREQUENCY = 1.0f;

	// Player target for projectiles to shoot at
	GameObject* playerTarget = nullptr;
	const float PROJECTILE_LIFETIME = 8.0f;

	BossComponent(GameObject* player, float speed = 150.0f, 
					float leftX = 1400.0f, float rightX = 1800.0f)
		: playerTarget(player), movementSpeed(speed),
			leftBound(leftX), rightBound(rightX) {}

	void update(GameObject& obj, float dt) override {
		updateMovement(obj, dt);
		updateLightAttack(obj, dt);
		updateHeavyAttack(obj, dt);
	}

private:

	void updateMovement(GameObject& obj, float dt) {
		
		auto* transform = obj.getComponent<TransformComponent>();
		if (!transform) return;

		Vec2 pos = transform->getPosition();

		// Move boss
		pos.x += direction * movementSpeed * dt;

		// Check bounds to switch direction
		if (pos.x <= leftBound) {
			pos.x = leftBound;
			direction = 1.0f;
		}
		else if (pos.x >= rightBound) {
			pos.x = rightBound;
			direction = -1.0f;
		}

		transform->setPosition(pos.x, pos.y);
	}

	void updateLightAttack(GameObject& obj, float dt) {

		if (lightAttackCooldown > 0) {
			lightAttackCooldown -= dt;
		}

		if (lightAttackCooldown <= 0 && playerTarget) {
			
			auto* bossTransform = obj.getComponent<TransformComponent>();
			auto* playerTransform = playerTarget->getComponent<TransformComponent>();

			if (bossTransform && playerTransform) {
				
				Vec2 bossPos = bossTransform->getPosition();
				Vec2 bossSize = bossTransform->getSize();
				Vec2 playerPos = playerTransform->getPosition();
				Vec2 playerSize = playerTransform->getSize();

				float startX = bossPos.x + bossSize.x / 2.0f;
				float startY = bossPos.y + bossSize.y / 2.0f;
				float targetX = playerPos.x + playerSize.x / 2.0f;
				float targetY = playerPos.y + playerSize.y / 2.0f;

				float dx = targetX - startX;
				float dy = targetY - startY;
				float length = std::sqrt(dx * dx + dy * dy);

				if (length > 0) {
					dx /= length;
					dy /= length;

					fireLightProjectile(startX, startY, dx, dy);
					lightAttackCooldown = LIGHT_ATTACK_RATE;
				}
			}
		}
	}

	void updateHeavyAttack(GameObject& obj, float dt) {
		if (heavyAttackCooldown > 0) {
			heavyAttackCooldown -= dt;
		}

		if (heavyAttackCooldown <= 0 && playerTarget) {
			auto* bossTransform = obj.getComponent<TransformComponent>();
			auto* playerTransform = playerTarget->getComponent<TransformComponent>();

			if (bossTransform && playerTransform) {
				Vec2 bossPos = bossTransform->getPosition();
				Vec2 bossSize = bossTransform->getSize();
				Vec2 playerPos = playerTransform->getPosition();
				Vec2 playerSize = playerTransform->getSize();

				float startX = bossPos.x + bossSize.x / 2.0f;
				float startY = bossPos.y + bossSize.y / 2.0f;
				float targetX = playerPos.x + playerSize.x / 2.0f;
				float targetY = playerPos.y + playerSize.y / 2.0f;

				float dx = targetX - startX;
				float dy = targetY - startY;
				float length = std::sqrt(dx * dx + dy * dy);

				if (length > 0) {
					dx /= length;
					dy /= length;

					fireHeavyProjectile(startX, startY, dx, dy);
					heavyAttackCooldown = HEAVY_ATTACK_RATE;
				}
			}
		}
	}

	void fireLightProjectile(float x, float y, float dirX, float dirY) {

		// CHECK POOL CAPACITY BEFORE SPAWNING
		float usagePercent = GameObjectAllocator::getPoolUsagePercent();
		if (usagePercent >= 95.0f) {
			SDL_Log("WARNING: Pool nearly full (%.1f%%), skipping light projectile spawn", usagePercent);
			return;
		}

		GameObject* projectile = GameObjectAllocator::create();
		if (!projectile) {
			SDL_Log("ERROR: Failed to create light projectile - pool is FULL!");
			return;
		}

		projectile->addComponent(std::make_unique<TagComponent>("boss_projectile"));
		projectile->addComponent(std::make_unique<TransformComponent>(
			x - 16.0f, y - 16.0f, 32.0f, 32.0f,
			dirX * LIGHT_PROJECTILE_SPEED, dirY * LIGHT_PROJECTILE_SPEED
		));

		projectile->addComponent(std::make_unique<ProjectileComponent>(
			PROJECTILE_LIFETIME, LIGHT_PROJECTILE_DAMAGE
		));

		projectile->addComponent(std::make_unique<ColliderComponent>());
		projectile->addComponent(std::make_unique<RenderComponent>("assets/skullFire.png"));

	Engine::addGameObject(projectile);

	}

	void fireHeavyProjectile(float x, float y, float dirX, float dirY) {

		// CHECK POOL CAPACITY BEFORE SPAWNING
		float usagePercent = GameObjectAllocator::getPoolUsagePercent();
		if (usagePercent >= 95.0f) {
			SDL_Log("WARNING: Pool nearly full (%.1f%%), skipping heavy projectile spawn", usagePercent);
			return;
		}

		GameObject* projectile = GameObjectAllocator::create();
		if (!projectile) {
			SDL_Log("ERROR: Failed to create heavy projectile - pool is FULL!");
			return;
		}

		projectile->addComponent(std::make_unique<TagComponent>("boss_projectile"));

		// Create sinusoidal projectile component
		auto sinProj = std::make_unique<SinusoidalProjectileComponent>(
			PROJECTILE_LIFETIME,
			HEAVY_PROJECTILE_DAMAGE,
			dirX * HEAVY_PROJECTILE_SPEED,
			dirY * HEAVY_PROJECTILE_SPEED,
			WAVE_AMPLITUDE,
			WAVE_FREQUENCY
		);

		projectile->addComponent(std::make_unique<TransformComponent>(
			x - 64.0f, y - 64.0f, 128.0f, 128.0f,
			dirX * HEAVY_PROJECTILE_SPEED, dirY * HEAVY_PROJECTILE_SPEED
		));
		projectile->addComponent(std::move(sinProj));
		projectile->addComponent(std::make_unique<ColliderComponent>());
		projectile->addComponent(std::make_unique<RenderComponent>("assets/Orb.png"));

		Engine::addGameObject(projectile);
	}

};