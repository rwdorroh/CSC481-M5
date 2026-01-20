#include <engine/RenderComponent.h>
#include <engine/TransformComponent.h>
#include <engine/GameObject.h>
#include <engine/Engine.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>

RenderComponent::RenderComponent(const std::string& texturePath, bool tile)
    : tileTexture(tile)
{

    SDL_Renderer* renderer = Engine::getRenderer();
    if (!renderer) {
        std::cerr << "[RenderComponent] Renderer is null!" << std::endl;
        return;
    }

    SDL_Surface* surface = IMG_Load(texturePath.c_str());
    if (!surface) {
        std::cerr << "[RenderComponent] Failed to load texture from " << texturePath << ": " << SDL_GetError() << std::endl;
        return;
    }

    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);

    if (!texture) {
        std::cerr << "[RenderComponent] Failed to create texture from " << texturePath << ": " << SDL_GetError() << std::endl;
    }
}

RenderComponent::~RenderComponent() {
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
}

void RenderComponent::draw(GameObject& obj, SDL_Renderer* renderer) {
    auto* transform = obj.getComponent<TransformComponent>();
    if (!transform || !texture) return;

    Vec2 pos = transform->getPosition();
    Vec2 size = transform->getSize();

    float texW = 0, texH = 0;
    SDL_GetTextureSize(texture, &texW, &texH);

    if (tileTexture) {
            for (float x = pos.x; x < pos.x + size.x; x += texW) {
                SDL_FRect dest = { 
                    x, 
                    pos.y,
                    static_cast<float>(texW),
                    static_cast<float>(texH) };
                SDL_RenderTexture(renderer, texture, nullptr, &dest);
            }
    } else {
        SDL_FRect dst = { pos.x, pos.y, size.x, size.y };
        SDL_RenderTexture(renderer, texture, nullptr, &dst);
    }
}

void RenderComponent::draw(GameObject& obj, SDL_Renderer* renderer, const Vec2& cameraOffset) {
    auto* transform = obj.getComponent<TransformComponent>();
    if (!transform || !texture) return;

    Vec2 pos = transform->getPosition();
    Vec2 size = transform->getSize();

    float texW = 0, texH = 0;
    SDL_GetTextureSize(texture, &texW, &texH);

    if (tileTexture) {
            for (float x = pos.x; x < pos.x + size.x; x += texW) {
                SDL_FRect dest = { 
                    x - cameraOffset.x, 
                    pos.y - cameraOffset.y, 
                    static_cast<float>(texW), 
                    static_cast<float>(texH) 
                };
                SDL_RenderTexture(renderer, texture, nullptr, &dest);
            }
    } else {
        SDL_FRect dst = { pos.x - cameraOffset.x, pos.y - cameraOffset.y, size.x, size.y };
        SDL_RenderTexture(renderer, texture, nullptr, &dst);
    }
}