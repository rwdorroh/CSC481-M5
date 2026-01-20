#pragma once
#include <engine/Engine.h>
#include <engine/Component.h>
#include <engine/TransformComponent.h>
#include <engine/ColliderComponent.h>
#include <engine/GameObject.h>

class ProjectileComponent : public Component {
public:
	float lifetime;
	float maxLifetime;
	int damage;
	bool hasHit = false;

	ProjectileComponent(float life = 2.0f, int dmg = 10)
		: lifetime(life), maxLifetime(life), damage(dmg) {}

	void update(GameObject& obj, float dt) override {
		lifetime -= dt;

		// Remove projectile when lifetime ends
		if (lifetime <= 0 || hasHit) {
			Engine::queueRemove(&obj);
		}
	}

	void onHit() {
		hasHit = true;
	}

	int getDamage() const {
		return damage;
	}
};