#pragma once
#include <engine/Component.h>
#include <engine/TransformComponent.h>
#include <engine/InputComponent.h>
#include <engine/RenderComponent.h>
#include <engine/GameObject.h>
#include "bossActions.h"
#include <iostream>

class DashComponent : public Component {

public:
	
	// Dash state
	bool isDashing = false;
	float dashTimer = 0.0f;
	float dashDirection = 0.0f; // -1 or 1
	float dashCooldown = 0.0f;

	// Dash params
	const float DASH_SPEED = 1000.0f;
	const float DASH_DURATION = 0.2f;
	const float DASH_COOLDOWN = 0.5f;

	// Original velocity to restore after dash
	Vec2 originalVelocity = { 0.0f, 0.0f };

	void update(GameObject& obj, float dt) override {

		// Update cooldown
		if (dashCooldown > 0) {
			dashCooldown -= dt;
		}

		auto* transform = obj.getComponent<TransformComponent>();
		if (!transform) return;

		// If dashing, apply dash velocity
		if (isDashing) {
			dashTimer -= dt;

			// If dash finished, restore original velocity
			if (dashTimer <= 0) {

				isDashing = false;
				transform->setVelocity(originalVelocity.x, originalVelocity.y);
				dashDirection = 0.0f;
				std::cout << "Dash!" << std::endl;

			}
			else {
				// Update transform with dash velocity
				transform->setVelocity(dashDirection * DASH_SPEED, 0.0f);
			}
		}
	}

	void tryStartDash(TransformComponent* transform, float direction) {
		if (dashCooldown > 0) return;
		if (isDashing) return;
		if (!transform) return;

		startDash(transform, direction);
	}

	bool isCurrentlyDashing() const {
		return isDashing;
	}

	bool canDash() const { return dashCooldown <= 0 && !isDashing; }

private:

	void startDash(TransformComponent* transform, float direction) {
		isDashing = true;
		dashTimer = DASH_DURATION;
		dashCooldown = DASH_COOLDOWN;
		dashDirection = direction;

		// Store current velocity
		originalVelocity = transform->getVelocity();

		// Set dash velocity
		transform->setVelocity(direction * DASH_SPEED, 0.0f);
	}
};