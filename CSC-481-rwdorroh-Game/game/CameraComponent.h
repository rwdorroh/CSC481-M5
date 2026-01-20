#pragma once
#include <engine/Component.h>
#include <engine/TransformComponent.h>
#include <engine/GameObject.h>
#include <algorithm>

class CameraComponent : public Component {
public:
	CameraComponent(float width, float height, float smooth = 0.1f)
		: viewWidth(width), viewHeight(height), smoothFactor(smooth) {
	}

	void setTarget(GameObject* targetObj) { target = targetObj; }

	void update(GameObject& /*owner*/, float deltaTime) override {
		if (!target) return;

		auto* t = target->getComponent<TransformComponent>();
		if (!t) return;

		Vec2 targetPos = t->getPosition();
		Vec2 targetSize = t->getSize();

		Vec2 desired{
			(targetPos.x + targetSize.x / 2.f) - viewWidth / 2.f,
			(targetPos.y + targetSize.y / 2.f) - viewHeight / 2.f
		};

		// Smooth follow (lerp)
		offset.x += (desired.x - offset.x) * std::min(1.f, deltaTime / smoothFactor);
		offset.y += (desired.y - offset.y) * std::min(1.f, deltaTime / smoothFactor);

		// Clamp to positive range (no negative scroll)
		offset.x = std::max(0.f, offset.x);
		offset.y = std::max(0.f, offset.y);
	}

	Vec2 getOffset() const { return offset; }

private:
	GameObject* target = nullptr;
	Vec2 offset{ 0.f, 0.f };
	float viewWidth, viewHeight;
	float smoothFactor;
};
