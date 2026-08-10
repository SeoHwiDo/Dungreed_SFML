#include "Projectile.h"
#include "ResourceManager.h"

#include <cmath>
#include <iostream>

void Projectile::activate(const ProjectileSpawnRequest& request) {
    m_active = true;
    m_isPlayingReturnTrail = false;
    m_isRangedWeapon = request.isRangedWeapon;
    m_animationKey = request.animationKey;
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

    if (m_isRangedWeapon) {
        if (m_animationKey.empty()) {
            std::cerr << "[Projectile] 원거리 투사체 애니메이션 키가 비어 있습니다.\n";
        } else {
            auto& resources = ResourceManager::getInstance();
            const std::string animationName = m_animationKey + "_Fly";
            const std::string firstFrameName = animationName + "-00.png";
            const sf::Texture* texture = resources.getAtlasTexture("Projectile");
            const sf::IntRect* frame =
                resources.getFrameRect("Projectile", firstFrameName);
            m_animationFrames =
                resources.getAnimationFrames("Projectile", animationName);
            if (texture != nullptr && frame != nullptr && m_animationFrames &&
                !m_animationFrames->empty()) {
                m_sprite.emplace(*texture);
                m_sprite->setTextureRect(*frame);
                m_sprite->setOrigin({
                    frame->size.x * 0.5f,
                    frame->size.y * 0.5f
                });
                m_sprite->setPosition(request.position);
            } else {
                std::cerr << "[Projectile] 투사체 Fly 리소스를 찾을 수 없습니다: "
                          << animationName << '\n';
            }
        }
    }

    m_collision.updateHitbox(getGlobalBounds());
}

void Projectile::update(float dt) {
    if (!m_active) {
        return;
    }

    if (m_isPlayingReturnTrail) {
        if (!m_animationFrames || m_animationFrames->empty() || !m_sprite) {
            m_active = false;
            return;
        }

        m_animationTime += dt;
        while (m_animationTime >= 0.1f) {
            m_animationTime -= 0.1f;
            ++m_animationFrame;
            if (m_animationFrame >= m_animationFrames->size()) {
                // 마지막 Trail 프레임이 표시된 뒤 다음 갱신에서 풀로 반환됩니다.
                m_active = false;
                return;
            }
            m_sprite->setTextureRect((*m_animationFrames)[m_animationFrame]);
        }
        return;
    }

    m_previousPosition = getPosition();
    m_shape.move(m_velocity * dt);
    if (m_sprite) {
        m_sprite->move(m_velocity * dt);
        if (m_animationFrames && !m_animationFrames->empty()) {
            m_animationTime += dt;
            while (m_animationTime >= 0.2f) {
                m_animationTime -= 0.2f;
                m_animationFrame = (m_animationFrame + 1) % m_animationFrames->size();
            }
            m_sprite->setTextureRect((*m_animationFrames)[m_animationFrame]);
        }
    }
    m_lifetime -= dt;
    m_collision.updateHitbox(getGlobalBounds());
    if (m_lifetime <= 0.f) {
        if (!beginReturnTrail()) {
            deactivate();
        }
    }
}

bool Projectile::beginReturnTrail() {
    if (!m_active || m_isPlayingReturnTrail || !m_isRangedWeapon ||
        m_animationKey.empty() || !m_sprite) {
        return false;
    }

    const std::vector<sf::IntRect>* trailFrames =
        ResourceManager::getInstance().getAnimationFrames(
            "Projectile", m_animationKey + "_Trail");
    if (!trailFrames || trailFrames->empty()) {
        return false;
    }

    m_isPlayingReturnTrail = true;
    m_velocity = { 0.f, 0.f };
    m_damage = 0.f;
    m_lifetime = 0.f;
    m_animationFrames = trailFrames;
    m_animationTime = 0.f;
    m_animationFrame = 0;
    m_sprite->setTextureRect((*m_animationFrames)[m_animationFrame]);
    m_collision.updateHitbox({});
    return true;
}

void Projectile::deactivate() {
    m_active = false;
    m_isPlayingReturnTrail = false;
    m_isRangedWeapon = false;
    m_animationKey.clear();
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
    return isDamageActive() && m_collision.checkHit(targetBounds).has_value();
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
