#pragma once

#include <SFML/Graphics.hpp>

#include <optional>

class Actor;
class TileMap;
class Projectile;

class Collision {
  public:
    Collision() = default;
    ~Collision() = default;

    void updateHitbox(const sf::FloatRect &actorBounds) { m_hitbox = actorBounds; }
    std::optional<sf::Vector2f> checkHit(const sf::FloatRect &attackBox) const;
    const sf::FloatRect &getHitbox() const { return m_hitbox; }

    static void resolveMapCollision(Actor &actor, const TileMap &map, bool ignoreOneWay = false);
    static bool resolveProjectileMapCollision(Projectile &projectile, const TileMap &map);

  private:
    sf::FloatRect m_hitbox;
};