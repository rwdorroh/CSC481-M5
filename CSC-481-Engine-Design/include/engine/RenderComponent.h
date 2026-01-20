#pragma once

#include "Component.h"
#include "TransformComponent.h"
#include <SDL3/SDL.h>
#include <string>

class RenderComponent : public Component {
public:
    RenderComponent(const std::string& texturePath, bool tile = false);
    ~RenderComponent();

    void draw(GameObject& obj, SDL_Renderer* renderer) override;
    
    void draw(GameObject& obj, SDL_Renderer* renderer, const Vec2& cameraOffset);

private:
    SDL_Texture* texture = nullptr;
    bool tileTexture = false;
};
