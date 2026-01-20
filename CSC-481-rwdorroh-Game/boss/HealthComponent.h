#pragma once
#include <engine/Component.h>
#include <engine/Engine.h>
#include <iostream>

class HealthComponent : public Component {

public:
	
	int maxHealth;
	int currentHealth;
	bool isDead = false;

	HealthComponent(int maxHp = 100)
		: maxHealth(maxHp), currentHealth(maxHp) {}

	void takeDamage(int damage) {
		
		if (isDead) return;

		currentHealth -= damage;
		std::cout << "Took " << damage << " damage! Health : " << currentHealth << "/" << maxHealth << std::endl;

		if (currentHealth <= 0) {
			currentHealth = 0;
			isDead = true;
		}
	}

	void heal(int amount) {
		if (isDead) return;
		currentHealth = std::min(currentHealth + amount, maxHealth);
	}

	void revive(int health = -1) {
		isDead = false;
		if (health < 0) {
			currentHealth = maxHealth;  // Full health by default
		}
		else {
			currentHealth = std::min(health, maxHealth);
		}
	}

	float getHealthPercent() const {
		return static_cast<float>(currentHealth) / static_cast<float>(maxHealth);
	}

	int getCurrentHealth() const { return currentHealth; }
	int getMaxHealth() const { return maxHealth; }

	bool isAlive() const {
		return !isDead;
	}

	void update(GameObject& obj, float dt) override {
		// Remove object if dead (optional)
		if (isDead && shouldRemoveOnDeath) {
			Engine::queueRemove(&obj);
		}
	}

	void setRemoveOnDeath(bool remove) { shouldRemoveOnDeath = remove; }

private:

	bool shouldRemoveOnDeath = false;

	void onDeath() {
		// std::cout << "Entity died!" << std::endl;
		// Can trigger events here if needed
	}
};