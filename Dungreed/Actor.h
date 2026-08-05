#pragma once
#include<SFML/Graphics.hpp>
#include <optional> // 추가
#include <string>   // 추가
#include"Animator.h"
#include "ResourceManager.h"
struct Status {
    float maxHp;
    float tmpHp;
    float power;//공격력
    float dex;//회피 및 치명타 확률
}; 
struct MovementData {
    sf::Vector2f velocity{ 0.f, 0.f };
    sf::Vector2f acceleration{ 0.f, 0.f };
    float moveSpeed = 300.f;
    float jumpForce = -500.f;
    float gravity = 980.f;
    bool isGrounded = false;
};
class Actor
{
public:
    Actor() = default;
    virtual ~Actor() = default;

    virtual void init(const std::string& atlasKey);
    void move(float dx, float dy);
    bool dead();
    float attack();
    //점프중일때 
    bool jump();
    sf::Vector2f getCenterPosition() const;
    void setHorizontalInput(float dirX);
    virtual void update(float dt);
    virtual void render(sf::RenderWindow& window);

protected:
    Status status;
    std::optional<sf::Sprite> sprite;
    Animator animator;


    MovementData movement;
    void updatePhysics(float dt);
private:
   
    void setBottomCenterOrigin();
};

