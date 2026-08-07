#include "Projectile.h"

#include <cmath>

void Projectile::activate(const ProjectileSpawnRequest& request) {
    m_active = true;
    m_target = request.target;
    m_previousPosition = request.position;
    m_velocity = request.direction * request.speed;
    m_damage = request.damage;
    m_lifetime = request.lifetime;
    m_shape.setPosition(request.position);
    m_shape.setOrigin({ 4.f, 4.f });
    m_shape.setFillColor(request.type == ProjectileType::Fireball
        ? sf::Color(255, 120, 40)
        : request.type == ProjectileType::Bullet
            ? sf::Color(220, 220, 220)
            : sf::Color(220, 190, 80));
    m_collision.updateHitbox(m_shape.getGlobalBounds());
}

void Projectile::update(float dt) {
    if (!m_active) {
        return;
    }

    m_previousPosition = m_shape.getPosition();
    m_shape.move(m_velocity * dt);
    m_lifetime -= dt;
    m_collision.updateHitbox(m_shape.getGlobalBounds());
    if (m_lifetime <= 0.f) {
        deactivate();
    }
}

void Projectile::deactivate() {
    m_active = false;
    m_velocity = { 0.f, 0.f };
    m_damage = 0.f;
    m_lifetime = 0.f;
    m_collision.updateHitbox({});
}

bool Projectile::checkHit(const sf::FloatRect& targetBounds) const {
    return m_active && m_collision.checkHit(targetBounds).has_value();
}

sf::FloatRect Projectile::getGlobalBoundsAt(const sf::Vector2f& position) const {
    sf::FloatRect bounds = m_shape.getGlobalBounds();
    const sf::Vector2f offset = position - m_shape.getPosition();
    bounds.position += offset;
    return bounds;
}

void Projectile::render(sf::RenderWindow& window) const {
    if (m_active) {
        window.draw(m_shape);
    }
}
