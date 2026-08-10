#pragma once

#include <SFML/Graphics.hpp>

#include <optional>
#include <string>
#include <unordered_set>

#include "Animator.h"
#include "Equip.h"

/// 풀에서 대여할 단일 이펙트의 재생·배치·공격 판정 정보를 묶습니다.
struct EffectSpawnRequest {
    std::string atlasKey = "Effect";
    std::string animationName;
    sf::Vector2f position{};
    float rotationRadian = 0.f;
    sf::Vector2f scale{ 1.f, 1.f };
    float frameDuration = 0.05f;
    bool isLoop = false;
    bool isAttackEffect = false;
    float damage = 0.f;
    sf::Vector2f attackerPosition{};
    sf::Color color = sf::Color::White;
};

/// 시각 이펙트와 공격 판정을 함께 표현하는, 재사용 가능한 풀 객체입니다.
class Effect {
public:
    /// 요청 데이터로 비활성 이펙트를 초기화해 재생을 시작합니다.
    bool activate(const EffectSpawnRequest& request);
    /// 애니메이션을 진행하고 마지막 프레임이 끝나면 비활성화 대기 상태로 전환합니다.
    void update(float dt);
    /// 현재 이펙트를 화면에 그립니다.
    void render(sf::RenderWindow& window) const;

    /// 공격 이펙트가 현재 몬스터와 한 번도 충돌하지 않았다면 true를 반환합니다.
    bool consumeHit(EntityId targetId);
    /// 공격 이펙트의 실제 스프라이트 범위를 반환합니다. 일반 이펙트는 nullopt입니다.
    std::optional<sf::FloatRect> getAttackHitbox() const;
    bool isFinished() const { return m_finished; }
    bool isAttackEffect() const { return m_isAttackEffect; }
    float getDamage() const { return m_damage; }
    const sf::Vector2f& getAttackerPosition() const { return m_attackerPosition; }
    float getRotationRadian() const { return m_rotationRadian; }

private:
    std::optional<sf::Sprite> m_sprite;
    Animator m_animator;
    std::unordered_set<EntityId> m_hitTargets;
    bool m_finished = true;
    bool m_isAttackEffect = false;
    float m_damage = 0.f;
    float m_rotationRadian = 0.f;
    sf::Vector2f m_attackerPosition{};
};
