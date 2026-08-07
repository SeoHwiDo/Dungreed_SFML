#pragma once

#include <SFML/Graphics.hpp>
#include "Collision.h"
#include "Equip.h"

/// 오브젝트 풀에서 재사용되는 투사체입니다. 생성 주체는 저장하지 않고 대상 그룹과 발사 설정만 보관합니다.
class Projectile {
public:
    Projectile() = default;

    /// 장비가 만든 요청으로 투사체를 활성화하고 위치·방향·피해량을 초기화합니다.
    void activate(const ProjectileSpawnRequest& request);
    /// 위치와 남은 수명을 갱신합니다. 벽 충돌은 ProjectileManager가 별도로 처리합니다.
    void update(float dt);
    /// 풀에 반환할 때 활성 상태와 런타임 데이터를 초기화합니다.
    void deactivate();

    /// 투사체가 아직 유효한지 반환합니다.
    bool isActive() const { return m_active; }
    /// 플레이어 또는 몬스터 중 투사체가 공격할 대상을 반환합니다.
    ProjectileTarget getTarget() const { return m_target; }
    /// 현재 투사체의 피해량을 반환합니다.
    float getDamage() const { return m_damage; }
    /// 현재 스프라이트의 월드 충돌 영역을 반환합니다.
    sf::FloatRect getGlobalBounds() const { return m_shape.getGlobalBounds(); }
    /// 지정한 위치에 놓였을 때의 투사체 경계를 계산합니다. 고속 투사체의 벽 통과를 검사할 때 사용합니다.
    sf::FloatRect getGlobalBoundsAt(const sf::Vector2f& position) const;
    /// 직전 프레임 위치를 반환합니다. 벽/고속 이동 충돌 검사에 사용합니다.
    sf::Vector2f getPreviousPosition() const { return m_previousPosition; }
    /// 현재 위치를 반환합니다.
    sf::Vector2f getPosition() const { return m_shape.getPosition(); }
    /// 투사체를 충돌 지점으로 이동시킵니다.
    void setPosition(const sf::Vector2f& position) { m_shape.setPosition(position); }
    /// 충돌 영역을 갱신한 뒤 공격 대상과 겹치는지 검사합니다.
    bool checkHit(const sf::FloatRect& targetBounds) const;
    /// 화면에 활성 투사체를 그립니다.
    void render(sf::RenderWindow& window) const;

private:
    bool m_active = false;
    ProjectileTarget m_target = ProjectileTarget::Monster;
    sf::Vector2f m_previousPosition;
    sf::Vector2f m_velocity;
    float m_damage = 0.f;
    float m_lifetime = 0.f;
    sf::CircleShape m_shape{ 4.f };
    Collision m_collision;
};
