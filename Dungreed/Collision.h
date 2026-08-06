#pragma once

#include <SFML/Graphics.hpp>
#include <optional>
class Actor;
class TileMap;

class Collision {
public:
    // 생성 시 이 콜리전 객체의 주인을 등록합니다.
    Collision()=default;
    ~Collision() = default;

    // 매 프레임 스프라이트 위치에 맞게 피격 박스(Hitbox)를 갱신합니다.
    inline void updateHitbox(const sf::FloatRect& actorBounds) { m_hitbox = actorBounds;}
    //피격 위치 확인 후 정확한 피격지점 반환, 없을 경우 nullopt반환
    std::optional<sf::Vector2f> checkHit(const sf::FloatRect& attackBox) const;

    inline const sf::FloatRect& getHitbox() const { return m_hitbox; }

    // 추가: 맵 충돌 처리 (Update -> Collision 분리)
    static void resolveMapCollision(Actor& actor, const TileMap& map);
private:
    sf::FloatRect m_hitbox;
};