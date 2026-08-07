#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <optional>
#include "ResourceManager.h"

class Actor; // 전방 선언

struct EquipStat {
    float damage = 10.f;      // 무기 공격력
    float attackSpeed = 1.f;  // 공격 속도
    float range = 50.f;       // 공격 사거리
};

class Equip {
public:
    //EquipStat : damage, attackSpeed, range
    Equip(const std::string& name, EquipStat stat);
    virtual ~Equip() = default;

    void init(const std::string& atlasKey, const std::string& frameName);

    inline void setOwner(Actor* owner) { m_owner = owner; }
    inline EquipStat getStat() const { return m_stat; }
    inline const std::string& getName() const { return m_name; }

    // 매 프레임 무기의 위치 및 조준점(Radian)에 따른 회전 갱신
    virtual void update(float dt, const sf::Vector2f& ownerPos, float aimRadian);

    virtual void render(sf::RenderWindow& window);

    // 타격 판정용 글로벌 좌표 히트박스 반환
    std::optional<sf::FloatRect> getAttackHitbox() const;

    // 공격 상태 진입을 위한 함수 추가
    void attack();
    inline bool isAttacking() const { return m_isAttacking; }
    // 한 번의 스윙으로 동일 대상에게 피해가 중복 적용되지 않도록 합니다.
    bool consumeHit();

protected:
    std::string m_name;
    EquipStat m_stat;
    std::optional<sf::Sprite> m_sprite;
    Actor* m_owner = nullptr; // 무기 소유자

    // 공격 애니메이션 및 상태 처리를 위한 변수
    bool m_isAttacking = false;
    bool m_hasDealtDamage = false;
    bool m_isSwung = false;   // 추가: 현재 무기가 꺾인 상태(토글)인지 확인
    float m_attackTimer = 0.f;
    float m_attackDuration = 0.5f;
};
