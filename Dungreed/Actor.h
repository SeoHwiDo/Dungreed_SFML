#pragma once
#include<SFML/Graphics.hpp>
#include <optional> 
#include <string>   
//-----------------------------------------------
#include"Animator.h"
#include "ResourceManager.h"
#include"Collision.h"

constexpr float MAXHP = 100.0f;
constexpr float POWER = 10.0f;
constexpr float DEX = 1.0f;

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
    struct Status {
        float maxHp;
        float tmpHp;
        float power;//공격력
        float dex;//회피 및 치명타 확률
    };
    Actor();
    Actor(Status _status) :status(_status){};

    virtual ~Actor() = default;

    virtual void init(const std::string& atlasKey);
    inline void move(float dx, float dy) { if (sprite)sprite->move({ dx,dy }); }
    inline bool dead() { return status.tmpHp <= 0; }
    inline float attack() { return status.power; };
    virtual void takeDamage(float damage) { status.tmpHp -= damage; };
    //점프중일때 true;
    bool jump();
    sf::Vector2f getCenterPosition() const;
    void setHorizontalInput(float dirX);

    // 외부에서 피격 판정 후 호출될 데미지 처리 함수
   

    virtual void update(float dt);
    virtual void render(sf::RenderWindow& window);
    

protected:
    Status status;
    std::optional<sf::Sprite> sprite;
    Animator animator;
    MovementData movement;
    // std::list<Item> Inventory;       
    // std::shared_ptr<Equip> equipment; 
    Collision col;
    void updatePhysics(float dt);
private:
   
    void setBottomCenterOrigin();
};

