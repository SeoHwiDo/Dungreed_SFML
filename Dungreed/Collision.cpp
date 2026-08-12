#include "Collision.h"

#include "Actor.h"
#include "Projectile.h"
#include "TileMap.h"

#include <cmath>

std::optional<sf::Vector2f> Collision::checkHit(const sf::FloatRect &attackBox) const {
    const auto intersection = m_hitbox.findIntersection(attackBox);
    if (!intersection)
        return std::nullopt;
    return sf::Vector2f{intersection->position.x + intersection->size.x / 2.f, intersection->position.y + intersection->size.y / 2.f};
}

void Collision::resolveMapCollision(Actor &actor, const TileMap &map, bool ignoreOneWay) {
    sf::FloatRect bounds = actor.getGlobalBounds();
    const sf::FloatRect previousBounds = actor.getPreviousGlobalBounds();
    auto &movement = actor.getMovement();
    movement.isGrounded = false;
    constexpr float groundEpsilon = 2.f;

    for (const auto &tile : map.getCollisionTiles()) {
        if (tile.type == TileType::None || (ignoreOneWay && tile.type == TileType::OneWay))
            continue;
        const auto intersection = bounds.findIntersection(tile.bounds);
        if (!intersection)
            continue;
        const sf::FloatRect &overlap = *intersection;

        if (tile.type == TileType::OneWay) {
            const float previousBottom = previousBounds.position.y + previousBounds.size.y;
            const float currentBottom = bounds.position.y + bounds.size.y;
            const bool crossedPlatformTop = previousBottom <= tile.bounds.position.y + groundEpsilon && currentBottom >= tile.bounds.position.y;
            if (movement.velocity.y > 0.f && crossedPlatformTop && bounds.position.y < tile.bounds.position.y) {
                actor.move(0.f, -overlap.size.y);
                movement.velocity.y = 0.f;
                movement.isGrounded = true;
                bounds = actor.getGlobalBounds();
            }
            continue;
        }

        if (overlap.size.x < overlap.size.y) {
            actor.move(bounds.position.x < tile.bounds.position.x ? -overlap.size.x : overlap.size.x, 0.f);
            movement.velocity.x = 0.f;
        } else if (movement.velocity.y >= 0.f && bounds.position.y < tile.bounds.position.y) {
            actor.move(0.f, -overlap.size.y);
            movement.velocity.y = 0.f;
            movement.isGrounded = true;
        } else {
            actor.move(0.f, overlap.size.y);
            movement.velocity.y = 0.f;
        }
        bounds = actor.getGlobalBounds();
    }

    bounds = actor.getGlobalBounds();
    const sf::FloatRect footCheck({bounds.position.x + 2.f, bounds.position.y + bounds.size.y}, {bounds.size.x - 4.f, groundEpsilon});
    movement.isGrounded = false;
    for (const auto &tile : map.getCollisionTiles()) {
        if (tile.type == TileType::None || (ignoreOneWay && tile.type == TileType::OneWay))
            continue;
        if (tile.type == TileType::OneWay) {
            const float previousBottom = previousBounds.position.y + previousBounds.size.y;
            if (movement.velocity.y < 0.f || previousBottom > tile.bounds.position.y + groundEpsilon)
                continue;
        }
        if (footCheck.findIntersection(tile.bounds)) {
            movement.isGrounded = true;
            break;
        }
    }
}

bool Collision::resolveProjectileMapCollision(Projectile &projectile, const TileMap &map) {
    if (!projectile.isDamageActive())
        return false;
    const sf::Vector2f start = projectile.getPreviousPosition();
    const sf::Vector2f end = projectile.getPosition();
    const sf::Vector2f delta = end - start;
    if (projectile.getType() == ProjectileType::BossSword) {
        // 보스 칼은 긴 스프라이트의 AABB가 중간 플랫폼이나 벽에 먼저 닿아
        // 멈추지 않도록, 진행 경로 아래에 있는 Solid 타일의 윗면만 판정합니다.
        // OneWay 타일은 의도적으로 모두 통과하며 바닥에만 꽂힙니다.
        if (delta.y <= 0.f) {
            return false;
        }

        float firstGroundHit = 2.f;
        for (const auto &tile : map.getCollisionTiles()) {
            if (tile.type != TileType::Solid || end.y < tile.bounds.position.y) {
                continue;
            }

            const float hitProgress = (tile.bounds.position.y - start.y) / delta.y;
            if (hitProgress < 0.f || hitProgress > 1.f || hitProgress >= firstGroundHit) {
                continue;
            }

            const float hitX = start.x + delta.x * hitProgress;
            if (hitX < tile.bounds.position.x || hitX > tile.bounds.position.x + tile.bounds.size.x) {
                continue;
            }
            firstGroundHit = hitProgress;
        }

        if (firstGroundHit <= 1.f) {
            projectile.setPosition(start + delta * firstGroundHit);
            return true;
        }
        return false;
    }

    const sf::Vector2f tileSize = map.getTileSize();
    const float distance = std::max(std::abs(delta.x), std::abs(delta.y));
    const unsigned int samples = std::max(1u, static_cast<unsigned int>(std::ceil(distance / std::max(1.f, std::max(tileSize.x, tileSize.y)))));
    for (unsigned int index = 1; index <= samples; ++index) {
        const float progress = static_cast<float>(index) / static_cast<float>(samples);
        const sf::Vector2f position = start + delta * progress;
        for (const auto &tile : map.getCollisionTiles()) {
            if (tile.type == TileType::None || tile.type == TileType::OneWay)
                continue;
            if (projectile.getGlobalBoundsAt(position).findIntersection(tile.bounds)) {
                projectile.setPosition(start + delta * (static_cast<float>(index - 1) / samples));
                return true;
            }
        }
    }
    return false;
}
