#include "Effect.h"

#include <iostream>

#include "ResourceManager.h"

bool Effect::activate(const EffectSpawnRequest& request) {
    auto& resourceManager = ResourceManager::getInstance();
    const sf::Texture* texture = resourceManager.getAtlasTexture(request.atlasKey);
    const std::vector<sf::IntRect>* frames = resourceManager.getAnimationFrames(
        request.atlasKey, request.animationName);
    if (!texture || !frames || frames->empty()) {
        std::cerr << "[Effect] 이펙트 애니메이션을 찾을 수 없습니다: "
            << request.atlasKey << '/' << request.animationName << '\n';
        return false;
    }

    m_sprite.emplace(*texture);
    m_sprite->setTextureRect(frames->front());
    if (const auto pivot = resourceManager.getAnimationFirstFramePivot(
        request.atlasKey, request.animationName)) {
        m_sprite->setOrigin(*pivot);
    } else {
        m_sprite->setOrigin({ frames->front().size.x * 0.5f, frames->front().size.y * 0.5f });
    }
    m_sprite->setPosition(request.position);
    m_sprite->setRotation(sf::radians(request.rotationRadian));
    m_sprite->setScale(request.scale);
    m_sprite->setColor(request.color);

    m_animator = Animator{};
    m_animator.addAnimation(request.animationName,
        AnimationClip(frames, request.frameDuration, request.isLoop));
    m_animator.play(request.animationName);
    m_hitTargets.clear();
    m_isAttackEffect = request.isAttackEffect;
    m_damage = request.damage;
    m_attackerPosition = request.attackerPosition;
    m_rotationRadian = request.rotationRadian;
    m_finished = false;
    return true;
}

void Effect::update(float dt) {
    if (m_finished) {
        return;
    }
    if (!m_sprite) {
        m_finished = true;
        return;
    }
    m_animator.update(dt, *m_sprite);
    if (m_animator.isFinished()) {
        m_finished = true;
    }
}

void Effect::render(sf::RenderWindow& window) const {
    if (!m_finished && m_sprite) {
        window.draw(*m_sprite);
    }
}

bool Effect::consumeHit(EntityId targetId) {
    if (m_finished || !m_isAttackEffect ||
        m_hitTargets.find(targetId) != m_hitTargets.end()) {
        return false;
    }
    m_hitTargets.insert(targetId);
    return true;
}

std::optional<sf::FloatRect> Effect::getAttackHitbox() const {
    if (m_finished || !m_isAttackEffect || !m_sprite) {
        return std::nullopt;
    }
    return m_sprite->getGlobalBounds();
}
