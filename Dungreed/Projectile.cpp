#include "Projectile.h"
#include "ResourceManager.h"

#include <cmath>

void Projectile::activate(const ProjectileSpawnRequest& request) {
    m_active = true;
    m_target = request.target;
    m_previousPosition = request.position;
    m_velocity = request.direction * request.speed;
    m_damage = request.damage;
    m_lifetime = request.lifetime;
    m_sprite.reset();
    m_animationFrames = nullptr;
    m_animationTime = 0.f;
    m_animationFrame = 0;

    m_shape.setPosition(request.position);
    m_shape.setOrigin({ 4.f, 4.f });
    m_shape.setFillColor(request.type == ProjectileType::Fireball
        ? sf::Color(255, 120, 40)
        : request.type == ProjectileType::Bullet
            ? sf::Color(220, 220, 220)
            : sf::Color(220, 190, 80));

    if (request.type == ProjectileType::BabyBatBullet) {
        auto& resources = ResourceManager::getInstance();
        const sf::Texture* texture = resources.getAtlasTexture("Projectile");
        const sf::IntRect* frame = resources.getFrameRect("Projectile", "BabyBatBullet_Fly-00.png");
        m_animationFrames = resources.getAnimationFrames("Projectile", "BabyBatBullet_Fly");
        if (texture != nullptr && frame != nullptr) {
            m_sprite.emplace(*texture);
            m_sprite->setTextureRect(*frame);
            m_sprite->setOrigin({ frame->size.x * 0.5f, frame->size.y * 0.5f });
            m_sprite->setPosition(request.position);
        }
    }

    m_collision.updateHitbox(getGlobalBounds());
}

void Projectile::update(float dt) {
    if (!m_active) {
        return;
    }

    m_previousPosition = getPosition();
    m_shape.move(m_velocity * dt);
    if (m_sprite) {
        m_sprite->move(m_velocity * dt);
        if (m_animationFrames && !m_animationFrames->empty()) {
            m_animationTime += dt;
            while (m_animationTime >= 0.1f) {
                m_animationTime -= 0.1f;
                m_animationFrame = (m_animationFrame + 1) % m_animationFrames->size();
            }
            m_sprite->setTextureRect((*m_animationFrames)[m_animationFrame]);
        }
    }
    m_lifetime -= dt;
    m_collision.updateHitbox(getGlobalBounds());
    if (m_lifetime <= 0.f) {
        deactivate();
    }
}

void Projectile::deactivate() {
    m_active = false;
    m_velocity = { 0.f, 0.f };
    m_damage = 0.f;
    m_lifetime = 0.f;
    m_sprite.reset();
    m_animationFrames = nullptr;
    m_animationTime = 0.f;
    m_animationFrame = 0;
    m_collision.updateHitbox({});
}

bool Projectile::checkHit(const sf::FloatRect& targetBounds) const {
    return m_active && m_collision.checkHit(targetBounds).has_value();
}

sf::FloatRect Projectile::getGlobalBoundsAt(const sf::Vector2f& position) const {
    sf::FloatRect bounds = getGlobalBounds();
    bounds.position += position - getPosition();
    return bounds;
}

sf::FloatRect Projectile::getGlobalBounds() const {
    return m_sprite ? m_sprite->getGlobalBounds() : m_shape.getGlobalBounds();
}

sf::Vector2f Projectile::getPosition() const {
    return m_sprite ? m_sprite->getPosition() : m_shape.getPosition();
}

void Projectile::setPosition(const sf::Vector2f& position) {
    m_shape.setPosition(position);
    if (m_sprite) {
        m_sprite->setPosition(position);
    }
}

void Projectile::render(sf::RenderWindow& window) const {
    if (!m_active) {
        return;
    }

    if (m_sprite) {
        window.draw(*m_sprite);
    }
    else {
        window.draw(m_shape);
    }
}