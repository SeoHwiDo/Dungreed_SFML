#include "Projectile.h"
#include "ResourceManager.h"

#include <algorithm>
#include <cmath>
#include <iostream>

void Projectile::activate(const ProjectileSpawnRequest& request) {
    m_active = true;
    m_isPlayingReturnTrail = false;
    m_isEmbedded = false;
    m_damageEnabled = request.damageActiveOnSpawn;
    m_isRangedWeapon = request.isRangedWeapon;
    m_type = request.type;
    m_animationKey = request.animationKey;
    m_returnAnimationKey = request.returnAnimationKey;
    m_rotateToDirection = request.rotateToDirection;
    m_rotationOffsetRadian = request.rotationOffsetRadian;
    m_target = request.target;
    m_previousPosition = request.position;
    m_direction = request.direction;
    setDirection(request.direction, request.speed);
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
        } else if (!loadVisual(m_animationKey)) {
            std::cerr << "[Projectile] 투사체 리소스를 찾을 수 없습니다: "
                      << m_animationKey << '\n';
        }
    }

    if (m_sprite) {
        m_sprite->setPosition(request.position);
        updateSpriteRotation();
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

    if (m_isEmbedded) {
        m_lifetime -= dt;
        if (m_lifetime <= 0.f) {
            deactivate();
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

    const std::string trailAnimation = m_returnAnimationKey.empty()
        ? m_animationKey + "_Trail"
        : m_returnAnimationKey;
    const std::vector<sf::IntRect>* trailFrames =
        ResourceManager::getInstance().getAnimationFrames(
            "Projectile", trailAnimation);
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
    const sf::FloatRect trailBounds = m_sprite->getLocalBounds();
    m_sprite->setOrigin({ trailBounds.size.x * 0.5f, trailBounds.size.y * 0.5f });
    m_collision.updateHitbox({});
    return true;
}

void Projectile::deactivate() {
    m_active = false;
    m_isPlayingReturnTrail = false;
    m_isEmbedded = false;
    m_damageEnabled = true;
    m_isRangedWeapon = false;
    m_animationKey.clear();
    m_returnAnimationKey.clear();
    m_direction = { 1.f, 0.f };
    m_velocity = { 0.f, 0.f };
    m_rotateToDirection = false;
    m_rotationOffsetRadian = 0.f;
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

void Projectile::setDirection(const sf::Vector2f& direction, float speed) {
    const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    m_direction = length > 0.001f ? direction / length : sf::Vector2f{ 0.f, 1.f };
    m_velocity = m_direction * speed;
    updateSpriteRotation();
}

void Projectile::updateSpriteRotation() {
    if (m_sprite && m_rotateToDirection) {
        const float angle = std::atan2(m_direction.y, m_direction.x) +
            m_rotationOffsetRadian;
        m_sprite->setRotation(sf::radians(angle));
    }
}

bool Projectile::loadVisual(const std::string& animationKey) {
    auto& resources = ResourceManager::getInstance();
    const sf::Texture* texture = resources.getAtlasTexture("Projectile");
    if (!texture) {
        return false;
    }

    std::string resolvedAnimation = animationKey + "_Fly";
    const std::vector<sf::IntRect>* frames =
        resources.getAnimationFrames("Projectile", resolvedAnimation);
    if (!frames || frames->empty()) {
        resolvedAnimation = animationKey;
        frames = resources.getAnimationFrames("Projectile", resolvedAnimation);
    }

    const sf::IntRect* frame = nullptr;
    if (frames && !frames->empty()) {
        frame = &frames->front();
        m_animationFrames = frames;
    } else {
        std::string frameName = animationKey;
        if (frameName.size() < 4 || frameName.substr(frameName.size() - 4) != ".png") {
            frameName += ".png";
        }
        frame = resources.getFrameRect("Projectile", frameName);
        m_animationFrames = nullptr;
    }
    if (!frame) {
        return false;
    }

    if (!m_sprite) {
        m_sprite.emplace(*texture);
    } else {
        m_sprite->setTexture(*texture);
    }
    m_sprite->setTextureRect(*frame);
    m_sprite->setOrigin({ frame->size.x * 0.5f, frame->size.y * 0.5f });
    m_animationTime = 0.f;
    m_animationFrame = 0;
    updateSpriteRotation();
    return true;
}

void Projectile::setAnimation(const std::string& animationKey) {
    m_animationKey = animationKey;
    loadVisual(animationKey);
}

void Projectile::embedInWall(float duration, float overlapPixels) {
    if (!m_active) {
        return;
    }
    setPosition(getPosition() + m_direction * std::max(0.f, overlapPixels));
    m_velocity = { 0.f, 0.f };
    m_damageEnabled = false;
    m_isEmbedded = true;
    m_lifetime = std::max(0.f, duration);
}

float Projectile::getRotationRadian() const {
    return m_sprite ? m_sprite->getRotation().asRadians() : 0.f;
}

void Projectile::render(sf::RenderWindow& window) const {
    if (!m_active) {
        return;
    }

    if (m_sprite) {
        if (m_isEmbedded && m_type == ProjectileType::BossSword) {
            return;
        }
        window.draw(*m_sprite);
    }
    else {
        window.draw(m_shape);
    }
}

void Projectile::renderBehindTiles(sf::RenderWindow& window) const {
    if (!m_active || !m_isEmbedded || m_type != ProjectileType::BossSword || !m_sprite) {
        return;
    }
    window.draw(*m_sprite);
}
