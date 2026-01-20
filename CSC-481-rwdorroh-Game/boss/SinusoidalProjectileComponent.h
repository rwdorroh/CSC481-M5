#pragma once
#include <engine/Component.h>
#include <engine/TransformComponent.h>
#include <engine/Engine.h>
#include <engine/GameObject.h>
#include <engine/GameObjectAllocator.hpp>

class SinusoidalProjectileComponent : public Component {

public:

	float lifetime;
	float maxLifetime;
	int damage;
	bool hasHit = false;

	float baseVelocityX;
	float baseVelocityY;
	float waveAmplitude;
	float waveFrequency;
	float timeAlive = 0.0f;

	SinusoidalProjectileComponent(float life, int dmg, float vx, float vy, float amplitude, float frequency)
		: lifetime(life), maxLifetime(life), damage(dmg),
		baseVelocityX(vx), baseVelocityY(vy),
		waveAmplitude(amplitude), waveFrequency(frequency) {
	}

	void update(GameObject& obj, float dt) override {

		lifetime -= dt;
		timeAlive += dt;

		if (lifetime <= 0 || hasHit) {
			Engine::queueRemove(&obj);
			return;
		}

		auto* transform = obj.getComponent<TransformComponent>();
		if (!transform) return;

		// Calculate perpendicular vector for wave
		float perpX, perpY;
		float baseLength = std::sqrt(baseVelocityX * baseVelocityX + baseVelocityY * baseVelocityY);

		if (baseLength > 0) {
			perpX = -baseVelocityY / baseLength;
			perpY = -baseVelocityX / baseLength;
		}
		else {
			perpX = 0.0f;
			perpY = 1.0f;
		}

		// Calculate wave offset
		float waveOffset = waveAmplitude * std::sin(2.0f * 3.14159f * waveFrequency * timeAlive);

		// Apply velocity with wave
		Vec2 vel;
		vel.x = baseVelocityX + perpX * waveOffset * waveFrequency * 2.0f * 3.14159f;
		vel.y = baseVelocityY + perpY * waveOffset * waveFrequency * 2.0f * 3.14159f;

		transform->setVelocity(vel.x, vel.y);

	}

	void onHit() { hasHit = true; }
	int getDamage() const { return damage; }
};
