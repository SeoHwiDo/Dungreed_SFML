#pragma once
#include<SFML/Graphics.hpp>
#include <optional> 
#include <string>   
//-----------------------------------------------
#include"Animator.h"
#include "ResourceManager.h"
#include"Collision.h"
#include"Equip.h"

class Equip;

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
    inline void move(float dx, float dy) { if (sprite) { sprite->move({ dx,dy }); col.updateHitbox(sprite->getGlobalBounds()); } }
    inline bool dead() const { return status.tmpHp <= 0; }
    inline float attack() const { return status.power; };
    virtual void takeDamage(float damage);
    virtual void takeDamage(float damage, const sf::Vector2f& attackerPosition);
    inline bool isHit() const { return m_hitTimer > 0.f; }
    //점프중일때 true;
    bool jump();
    sf::Vector2f getCenterPosition() const;
    sf::Vector2f getBodyCenterPosition() const;
    void setHorizontalInput(float dirX);

    // 장비 장착 및 관리 함수
    void setEquipment(std::shared_ptr<Equip> eq);
    inline std::shared_ptr<Equip> getEquipment() const { return equipment; }

    // 타격(공격) 판정 영역 반환 (무기가 없으면 자신의 스프라이트 반환)
    std::optional<sf::FloatRect> getAttackHitbox() const;


    // 추가: 충돌 연산을 위해 물리/트랜스폼 데이터 노출
    inline sf::Vector2f getPosition() const { return sprite ? sprite->getPosition() : sf::Vector2f{ 0.f, 0.f }; }
    inline sf::FloatRect getGlobalBounds() const { return sprite ? sprite->getGlobalBounds() : sf::FloatRect{}; }
    inline MovementData& getMovement() { return movement; }
    inline const Collision& getCollision() const { return col; }

    virtual void update(float dt);
    virtual void render(sf::RenderWindow& window);
    

protected:
    Status status;
    std::optional<sf::Sprite> sprite;
    Animator animator;
    MovementData movement;
    // std::list<Item> Inventory;       
    std::shared_ptr<Equip> equipment; 
    Collision col;
    void updatePhysics(float dt);
    void updateAnimation(float dt);
    std::string m_currentAnimation;

    void playAnimation(const std::string& animationName);
    void updateHitFeedback(float dt);

    float m_hitTimer = 0.f;
    float m_knockbackTimer = 0.f;
    sf::Vector2f m_knockbackVelocity{ 0.f, 0.f };
    static constexpr float HIT_COLOR_DURATION = 1.f;
    static constexpr float KNOCKBACK_DURATION = 0.12f;
    static constexpr float KNOCKBACK_SPEED = 350.f;

private:
   
    void setBottomCenterOrigin();
};

