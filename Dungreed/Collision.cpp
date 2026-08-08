#include "Collision.h"
#include "Actor.h"
#include "TileMap.h"
#include "Projectile.h"
#include <cmath>

std::optional<sf::Vector2f> Collision::checkHit(const sf::FloatRect& attackBox) const {
    const auto intersection = m_hitbox.findIntersection(attackBox);
    if (!intersection) return std::nullopt;
    return sf::Vector2f{ intersection->position.x + intersection->size.x / 2.f,
        intersection->position.y + intersection->size.y / 2.f };
}

void Collision::resolveMapCollision(Actor& actor, const TileMap& map, bool ignoreOneWay) {
    sf::FloatRect bounds = actor.getGlobalBounds();
    auto& movement = actor.getMovement();
    movement.isGrounded = false;
    constexpr float GroundEpsilon = 2.f;

    for (const auto& tile : map.getCollisionTiles()) {
        if (tile.type == TileType::None || (ignoreOneWay && tile.type == TileType::OneWay)) continue;
        const auto intersection = bounds.findIntersection(tile.bounds);
        if (!intersection) continue;
        const sf::FloatRect& overlap = *intersection;

        if (tile.type == TileType::OneWay) {
            if (movement.velocity.y >= 0.f && bounds.position.y < tile.bounds.position.y) {
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
    const sf::FloatRect footCheck({ bounds.position.x + 2.f, bounds.position.y + bounds.size.y }, { bounds.size.x - 4.f, GroundEpsilon });
    movement.isGrounded = false;
    for (const auto& tile : map.getCollisionTiles()) {
        if (tile.type == TileType::None || (ignoreOneWay && tile.type == TileType::OneWay) ||
            (tile.type == TileType::OneWay && movement.velocity.y < 0.f)) continue;
        if (footCheck.findIntersection(tile.bounds)) { movement.isGrounded = true; break; }
    }
}

bool Collision::resolveProjectileMapCollision(Projectile& projectile, const TileMap& map) {
    if (!projectile.isActive()) return false;
    const sf::Vector2f start = projectile.getPreviousPosition();
    const sf::Vector2f end = projectile.getPosition();
    const sf::Vector2f delta = end - start;
    const sf::Vector2f tileSize = map.getTileSize();
    const float distance = std::max(std::abs(delta.x), std::abs(delta.y));
    const unsigned int samples = std::max(1u, static_cast<unsigned int>(std::ceil(distance / std::max(1.f, std::max(tileSize.x, tileSize.y)))));
    for (unsigned int i = 1; i <= samples; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(samples);
        const sf::Vector2f position = start + delta * t;
        for (const auto& tile : map.getCollisionTiles()) {
            if (tile.type == TileType::None || tile.type == TileType::OneWay) continue;
            if (projectile.getGlobalBoundsAt(position).findIntersection(tile.bounds)) {
                projectile.setPosition(start + delta * (static_cast<float>(i - 1) / samples));
                return true;
            }
        }
    }
    return false;
}