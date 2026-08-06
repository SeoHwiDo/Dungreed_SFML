#include "Collision.h"
#include "Actor.h"
#include "TileMap.h"
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

void Collision::resolveMapCollision(Actor& actor, const TileMap& map) {
    sf::FloatRect bounds = actor.getGlobalBounds();

    auto& movement = actor.getMovement();

    movement.isGrounded = false;

    constexpr float GroundEpsilon = 2.f;

    for (const auto& tile : map.getCollisionTiles()) {
        if (tile.type == TileType::None) continue;

        // SFML 3.1.0 기준 반환값 optional 
        auto intersection = bounds.findIntersection(tile.bounds);
        if (!intersection)
            continue;

        const sf::FloatRect& overlap = *intersection;

        if (overlap.size.x < overlap.size.y) {
            if (bounds.position.x < tile.bounds.position.x)
                actor.move(-overlap.size.x, 0.f);
            else
                actor.move(overlap.size.x, 0.f);

            movement.velocity.x = 0.f;
        } else {
            // 위에서 떨어짐
            if (movement.velocity.y >= 0 &&
                bounds.position.y < tile.bounds.position.y) {
                actor.move(0.f, -overlap.size.y);

                movement.velocity.y = 0.f;
                movement.isGrounded = true;
            }
            // 아래에서 충돌
            else if (tile.type == TileType::Solid) {
                actor.move(0.f, overlap.size.y);

                movement.velocity.y = 0.f;
            }
        }

        bounds = actor.getGlobalBounds();
    }
    bounds = actor.getGlobalBounds();

    sf::FloatRect footCheck(
        { bounds.position.x + 2.f,
          bounds.position.y + bounds.size.y },
        { bounds.size.x - 4.f,
          GroundEpsilon });

    movement.isGrounded = false;

    for (const auto& tile : map.getCollisionTiles()) {
        if (tile.type == TileType::None)
            continue;

        if (footCheck.findIntersection(tile.bounds)) {
            movement.isGrounded = true;
            break;
        }
    }
}