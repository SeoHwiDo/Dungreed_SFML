#pragma once

#include <SFML/Graphics.hpp>
#include <optional>
class Actor;
class TileMap;
class Projectile;

class Collision {
public:
    /// 빈 충돌 상자로 생성합니다. Actor::init 또는 이동 시 updateHitbox가 실제 경계를 설정합니다.
    Collision()=default;
    ~Collision() = default;

    /// 스프라이트의 월드 경계로 피격 상자를 동기화합니다. 이동 후 반드시 호출합니다.
    inline void updateHitbox(const sf::FloatRect& actorBounds) { m_hitbox = actorBounds;}
    /// 공격 사각형과 겹치는지 검사하고, 겹치면 교차 영역의 중심 좌표를 반환합니다.
    std::optional<sf::Vector2f> checkHit(const sf::FloatRect& attackBox) const;

    /// 가장 최근에 동기화된 피격 상자를 반환합니다.
    inline const sf::FloatRect& getHitbox() const { return m_hitbox; }

    /// 액터와 TileMap의 Solid/OneWay 타일 충돌을 해결하고 착지 상태를 갱신합니다. 물리 업데이트 뒤 호출합니다.
    static void resolveMapCollision(Actor& actor, const TileMap& map);
    /// 투사체의 이전 위치부터 현재 위치까지를 잘게 샘플링해 벽을 먼저 판정합니다.
    static bool resolveProjectileMapCollision(Projectile& projectile, const TileMap& map);
private:
    sf::FloatRect m_hitbox;
};
