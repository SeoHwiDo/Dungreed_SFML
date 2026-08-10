#include "EffectManager.h"

#include <cmath>
#include <vector>

#include "ObjectPoolingManager.h"

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kHalfPi = kPi * 0.5f;
constexpr float kSwingForwardOffset = 16.f;
constexpr float kSwingFrameDuration = 0.05f;
constexpr float kSlashFrameDuration = 0.06f;
constexpr float kMagicCircleFrameDuration = 0.06f;
constexpr int kMagicCircleFrameCount = 15;
constexpr float kMagicCircleRevealDelay =
    kMagicCircleFrameDuration * kMagicCircleFrameCount * 0.5f;
}

void EffectManager::spawnPlayerSwing(ObjectPoolingManager& objectPool,
    const sf::Vector2f& playerPosition, float aimRadian, float damage) {
    const sf::Vector2f direction{ std::cos(aimRadian), std::sin(aimRadian) };
    objectPool.acquireEffect({
        "Effect", "SwingFX",
        playerPosition + direction * kSwingForwardOffset,
        // SwingFX 원본의 위쪽(-Y)을 현재 조준 방향으로 맞춥니다.
        aimRadian + kHalfPi,
        { 1.f, 1.f },
        kSwingFrameDuration,
        false,
        true,
        damage,
        playerPosition
    });
}

void EffectManager::spawnHitSlash(ObjectPoolingManager& objectPool,
    const sf::Vector2f& hitPosition, float rotationRadian) {
    objectPool.acquireEffect({
        "Effect", "SlashFX",
        hitPosition,
        rotationRadian,
        { 1.f, 1.f },
        kSlashFrameDuration,
        false,
        false,
        0.f,
        hitPosition
    });
}

float EffectManager::spawnMonsterMagicCircle(ObjectPoolingManager& objectPool,
    const sf::Vector2f& monsterPosition) {
    objectPool.acquireEffect({
        "Effect", "MagicCircleFx",
        monsterPosition,
        0.f,
        { 1.15f, 1.15f },
        kMagicCircleFrameDuration,
        false,
        false,
        0.f,
        monsterPosition,
        sf::Color::White
    });
    return kMagicCircleRevealDelay;
}

void EffectManager::spawnRewardChestMagicCircle(ObjectPoolingManager& objectPool,
    const sf::Vector2f& chestPosition) {
    objectPool.acquireEffect({
        "Effect", "MagicCircleFx",
        chestPosition,
        0.f,
        { 1.15f, 1.15f },
        kMagicCircleFrameDuration,
        false,
        false,
        0.f,
        chestPosition,
        sf::Color::White
    });
}

void EffectManager::update(float dt, ObjectPoolingManager& objectPool) {
    std::vector<Effect*> expiredEffects;
    objectPool.forEachActiveEffect([&](Effect& effect) {
        effect.update(dt);
        if (effect.isFinished()) {
            expiredEffects.push_back(&effect);
        }
    });
    for (Effect* effect : expiredEffects) {
        objectPool.releaseEffect(effect);
    }
}

void EffectManager::forEachActiveAttackEffect(const ObjectPoolingManager& objectPool,
    const std::function<void(Effect&)>& operation) const {
    objectPool.forEachActiveEffect([&](Effect& effect) {
        if (effect.isAttackEffect()) {
            operation(effect);
        }
    });
}

void EffectManager::render(sf::RenderWindow& window,
    const ObjectPoolingManager& objectPool) const {
    objectPool.forEachActiveEffect([&](Effect& effect) {
        effect.render(window);
    });
}

void EffectManager::clear(ObjectPoolingManager& objectPool) {
    std::vector<Effect*> activeEffects;
    objectPool.forEachActiveEffect([&](Effect& effect) {
        activeEffects.push_back(&effect);
    });
    for (Effect* effect : activeEffects) {
        objectPool.releaseEffect(effect);
    }
}
