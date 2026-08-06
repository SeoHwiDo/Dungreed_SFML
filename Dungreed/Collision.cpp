#include "Collision.h"
#include<optional>
std::optional<sf::Vector2f> Collision::checkHit(const sf::FloatRect& attackBox) const {
    auto intersection = m_hitbox.findIntersection(attackBox);

    if (intersection.has_value()) {
        sf::FloatRect overlap = intersection.value();

        // 겹친 영역의 정중앙 좌표를 계산하여 반환
        return sf::Vector2f{
            overlap.position.x + (overlap.size.x / 2.f),
            overlap.position.y + (overlap.size.y / 2.f)
        };
    }

    return std::nullopt; // 충돌하지 않음
}