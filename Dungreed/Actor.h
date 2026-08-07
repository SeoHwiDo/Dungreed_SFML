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
    /// 기본 능력치로 액터를 생성하고, 이후 init으로 아틀라스 스프라이트를 준비합니다.
    Actor();
    /// 지정한 능력치로 액터를 생성합니다. 파생 클래스 생성자에서 초기 능력치를 넘길 때 사용합니다.
    Actor(Status _status) :status(_status){};

    virtual ~Actor() = default;

    /// 아틀라스 키로 기본 스프라이트와 충돌 상자를 초기화합니다. 파생 클래스는 애니메이션 등록 후 호출합니다.
    virtual void init(const std::string& atlasKey);
    /// 월드 좌표만큼 이동시키고 이동 직후 피격 상자를 동기화합니다. 물리 보정 없이 순간 이동할 때 사용합니다.
    inline void move(float dx, float dy) { if (sprite) { sprite->move({ dx,dy }); col.updateHitbox(sprite->getGlobalBounds()); } }
    /// 현재 체력이 0 이하인지 반환합니다. 사망 상태 전환과 입력 차단의 기준으로 사용합니다.
    inline bool dead() const { return status.tmpHp <= 0; }
    /// 기본 공격력 값을 반환합니다. 장비 공격력과 함께 피해량 계산에 사용합니다.
    inline float attack() const { return status.power; };
    /// 공격자 위치를 모를 때 체력과 피격 효과만 적용합니다.
    virtual void takeDamage(float damage);
    /// 공격자 위치를 이용해 반대 방향 넉백과 피격 효과를 함께 적용합니다.
    virtual void takeDamage(float damage, const sf::Vector2f& attackerPosition);
    /// 피격 색상/넉백 타이머가 남아 있는지 반환합니다. 피격 중복 처리 제한에 사용합니다.
    inline bool isHit() const { return m_hitTimer > 0.f; }
    /// 지면 위일 때만 위쪽 초기 속도를 부여합니다. 점프가 실제로 시작되면 true를 반환합니다.
    bool jump();
    /// 스프라이트 전체 사각형의 중앙 월드 좌표를 반환합니다. 조준 및 거리 계산에 사용합니다.
    sf::Vector2f getCenterPosition() const;
    /// 시각적 여백을 제외한 몸통 중심 좌표를 반환합니다. 무기 배치 기준점으로 사용합니다.
    sf::Vector2f getBodyCenterPosition() const;
    /// 좌우 입력값(-1~1)을 수평 속도로 반영합니다. 각 상태 머신의 이동 처리에서 호출합니다.
    void setHorizontalInput(float dirX);

    /// 장비의 소유자를 이 액터로 연결하고 기존 장비를 교체합니다.
    void setEquipment(std::shared_ptr<Equip> eq);
    /// 현재 장착 장비를 반환합니다. 장비가 없으면 nullptr을 반환합니다.
    inline std::shared_ptr<Equip> getEquipment() const { return equipment; }

    /// 현재 공격 판정 영역을 반환합니다. 무기가 있으면 무기 판정, 없으면 본체 영역을 사용합니다.
    std::optional<sf::FloatRect> getAttackHitbox() const;


    /// 현재 스프라이트의 기준 위치를 반환합니다. 스프라이트가 없으면 원점을 반환합니다.
    inline sf::Vector2f getPosition() const { return sprite ? sprite->getPosition() : sf::Vector2f{ 0.f, 0.f }; }
    /// 현재 스프라이트의 월드 경계를 반환합니다. 맵 충돌 검사에서 사용합니다.
    inline sf::FloatRect getGlobalBounds() const { return sprite ? sprite->getGlobalBounds() : sf::FloatRect{}; }
    /// 속도·중력·착지 상태를 수정할 수 있는 이동 데이터를 반환합니다. 충돌 해결기에서 사용합니다.
    inline MovementData& getMovement() { return movement; }
    /// 현재 피격 상자를 읽기 전용으로 반환합니다. 공격 충돌 검사에서 사용합니다.
    inline const Collision& getCollision() const { return col; }

    /// 물리, 피격 효과, 애니메이션을 순서대로 갱신합니다. 파생 클래스의 공통 프레임 처리로 호출합니다.
    virtual void update(float dt);
    /// 본체 스프라이트와 장착 장비를 창에 그립니다.
    virtual void render(sf::RenderWindow& window);
    

protected:
    Status status;
    std::optional<sf::Sprite> sprite;
    Animator animator;
    MovementData movement;
    // std::list<Item> Inventory;       
    std::shared_ptr<Equip> equipment; 
    Collision col;
    /// 중력·일반 속도·넉백 속도를 합쳐 이동하고 피격 상자를 갱신합니다.
    void updatePhysics(float dt);
    /// 선택된 애니메이터를 한 프레임 진행시킵니다.
    void updateAnimation(float dt);
    std::string m_currentAnimation;

    /// 현재 재생 중인 이름과 다를 때만 애니메이션을 재시작합니다.
    void playAnimation(const std::string& animationName);
    /// 피격 색상 및 넉백 지속 시간을 감소시키고 종료 상태를 복구합니다.
    void updateHitFeedback(float dt);

    float m_hitTimer = 0.f;
    float m_knockbackTimer = 0.f;
    sf::Vector2f m_knockbackVelocity{ 0.f, 0.f };
    static constexpr float HIT_COLOR_DURATION = 1.f;
    static constexpr float KNOCKBACK_DURATION = 0.12f;
    static constexpr float KNOCKBACK_SPEED = 350.f;

private:
   
    /// 발밑 중앙을 스프라이트 원점으로 설정해 위치와 충돌 기준을 통일합니다.
    void setBottomCenterOrigin();
};

