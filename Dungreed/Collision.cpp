#include "Collision.h"
#include "Actor.h"
#include "TileMap.h"
#include "Projectile.h"
#include <cmath>
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

bool Collision::resolveProjectileMapCollision(Projectile& projectile, const TileMap& map) {
    if (!projectile.isActive()) {
        return false;
    }

    const sf::Vector2f start = projectile.getPreviousPosition();
    const sf::Vector2f end = projectile.getPosition();
    const sf::Vector2f delta = end - start;
    const sf::Vector2f tileSize = map.getTileSize();
    const float sampleLength = std::max(1.f, std::max(tileSize.x, tileSize.y));
    const float distance = std::max(std::abs(delta.x), std::abs(delta.y));
    const unsigned int samples = std::max(1u, static_cast<unsigned int>(std::ceil(distance / sampleLength)));

    for (unsigned int i = 1; i <= samples; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(samples);
        const sf::Vector2f position = start + delta * t;
        const sf::FloatRect bounds = projectile.getGlobalBoundsAt(position);
        for (const auto& tile : map.getCollisionTiles()) {
            if (tile.type == TileType::None) {
                continue;
            }
            if (bounds.findIntersection(tile.bounds).has_value()) {
                const float safeT = static_cast<float>(i - 1) / static_cast<float>(samples);
                projectile.setPosition(start + delta * safeT);
                return true;
            }
        }
    }
    return false;
}
