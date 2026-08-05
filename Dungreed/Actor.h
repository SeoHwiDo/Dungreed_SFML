#pragma once
#include<SFML/Graphics.hpp>
#include"Animator.h"
#include "ResourceManager.h"
struct Status {
    float maxHp;
    float tmpHp;
    float power;//공격력
    float dex;//회피 및 치명타 확률
};
class Actor
{
public:
    Actor() = default;
    virtual ~Actor() = default;

    virtual void init(const std::string& atlasKey) {
        auto& resourceManager = ResourceManager::getInstance();
        const sf::Texture* tex = resourceManager.getAtlasTexture(atlasKey);
        if (tex) {
            if (!sprite.has_value()) {
                sprite.emplace();
            }
            sprite->setTexture(*tex);
            setBottomCenterOrigin();
        }
    }
    void move(float x, float y);
    bool dead();
    float attack();
    bool jump();

    virtual void update(float dt);
    virtual void render(sf::RenderWindow& window);
protected:
    Status status;
    std::optional<sf::Sprite> sprite;
    Animator animator;
private:
    Status status;
    void setBottomCenterOrigin();
};

