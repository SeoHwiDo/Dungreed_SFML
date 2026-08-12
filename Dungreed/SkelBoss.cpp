#include "SkelBoss.h"

#include "EffectManager.h"
#include "GameDataManager.h"
#include "ObjectPoolingManager.h"
#include "Player.h"
#include "ResourceManager.h"
#include "TileMap.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kBossScale = 1.5f;
constexpr float kHandEdgeInsetTiles = 5.f;
constexpr float kHandIdleAmplitude = 42.f;
constexpr float kHandMoveSpeed = 520.f;
constexpr float kLaserTelegraphDuration = 0.55f;
constexpr float kLaserFireDuration = 0.35f;
constexpr float kLaserWidth = 24.f;
constexpr float kBulletPatternDuration = 4.1f;
constexpr float kMinimumBulletInterval = 0.01f;
constexpr float kBulletRotationSpeed = 0.375f;
constexpr float kSwordFanHalfRadian = 0.95f;
constexpr float kBackParticleInterval = 0.18f;
constexpr float kBackParticleHorizontalRadius = 48.f;
constexpr float kBackParticleVerticalRadius = 42.f;
} // namespace

SkelBoss::SkelBoss() : Boss("", {1200.f, 1200.f, 18.f, 1.f}) {
    setDisplayName(kUiDisplayName);
    configurePatternWeapons();
    init("Boss");
    beginSummon();
}

void SkelBoss::init(const std::string &atlasKey) {
    Boss::init(atlasKey);
    configureAnimations();
    configureHands();
}

void SkelBoss::configurePatternWeapons() {
    const auto &gameData = GameDataManager::getInstance();
    m_handLaserWeapon = gameData.createEquip("SkelBossHandLaser");
    m_bulletWeapon = gameData.createEquip("SkelBossRotatingBullet");
    m_swordWeapon = gameData.createEquip("SkelBossSword");

    if (!m_bulletWeapon) {
        std::cerr << "[SkelBoss] Bullet weapon could not be created\n";
    }
    if (!m_swordWeapon) {
        std::cerr << "[SkelBoss] Sword weapon could not be created\n";
    }
}

void SkelBoss::configureAnimations() {
    auto &resources = ResourceManager::getInstance();
    const sf::Texture *texture = resources.getAtlasTexture("Boss");
    const auto *idleFrames = resources.getAnimationFrames("Boss", "SkellBoss_Idle");
    const auto *bossBackFrames = resources.getAnimationFrames("Boss", "SkellBoss_Back");
    const auto *attackFrames = resources.getAnimationFrames("Boss", "SkellBoss_Attack");
    animator.addAnimation("SkellBoss_Idle", AnimationClip(idleFrames, 0.12f, true));
    animator.addAnimation("SkellBoss_Attack", AnimationClip(attackFrames, 0.09f, true));
    if (sprite && idleFrames && !idleFrames->empty()) {
        sprite->setTextureRect(idleFrames->front());
        const sf::FloatRect bounds = sprite->getLocalBounds();
        sprite->setOrigin({bounds.size.x * 0.5f, bounds.size.y});
        sprite->setScale({kBossScale, kBossScale});
    }
    animator.play("SkellBoss_Idle");

    if (texture && bossBackFrames && !bossBackFrames->empty()) {
        m_backSprite.emplace(*texture);
        m_backSprite->setTextureRect(bossBackFrames->front());
        m_backSprite->setOrigin(m_backSprite->getLocalBounds().getCenter());
        m_backSprite->setScale({kBossScale, kBossScale});
        m_backAnimator.addAnimation("Back", AnimationClip(bossBackFrames, 0.12f, true));
        m_backAnimator.play("Back");
    }
}

void SkelBoss::updateBackVisual(float dt) {
    if (!m_backSprite) {
        return;
    }
    m_backAnimator.update(dt, *m_backSprite);
    m_backSprite->setOrigin(m_backSprite->getLocalBounds().getCenter());
    m_backSprite->setPosition(getMouthPosition());
}

void SkelBoss::updateBackParticles(float dt, ObjectPoolingManager &objectPool) {
    if (!m_backSprite) {
        return;
    }

    m_backParticleTimer += dt;
    std::uniform_real_distribution<float> angleDistribution(0.f, kPi * 2.f);
    std::uniform_real_distribution<float> radiusDistribution(0.55f, 1.f);
    while (m_backParticleTimer >= kBackParticleInterval) {
        m_backParticleTimer -= kBackParticleInterval;
        const float angle = angleDistribution(m_backParticleRandom);
        const float radius = radiusDistribution(m_backParticleRandom);
        const sf::Vector2f position = m_backSprite->getPosition() + sf::Vector2f{std::cos(angle) * kBackParticleHorizontalRadius * radius, std::sin(angle) * kBackParticleVerticalRadius * radius};
        objectPool.acquireEffect({"Boss", "SkellBoss_Particle", position, angle, {0.75f, 0.75f}, 0.055f, false, false, 0.f, position, sf::Color::White});
    }
}

void SkelBoss::configureHands() {
    auto &resources = ResourceManager::getInstance();
    const sf::Texture *texture = resources.getAtlasTexture("Boss");
    const auto *idleFrames = resources.getAnimationFrames("Boss", "SkellBoss_LeftHandIdle");
    const auto *attackFrames = resources.getAnimationFrames("Boss", "SkellBoss_LeftHandAttack");
    if (!texture || !idleFrames || idleFrames->empty() || !attackFrames || attackFrames->empty()) {
        return;
    }

    m_leftHand.emplace(*texture);
    m_rightHand.emplace(*texture);
    m_leftHand->setTextureRect(idleFrames->front());
    m_rightHand->setTextureRect(idleFrames->front());
    m_leftHandAnimator.addAnimation("Idle", AnimationClip(idleFrames, 0.1f, true));
    m_leftHandAnimator.addAnimation("Attack", AnimationClip(attackFrames, 0.06f, true));
    m_rightHandAnimator.addAnimation("Idle", AnimationClip(idleFrames, 0.1f, true));
    m_rightHandAnimator.addAnimation("Attack", AnimationClip(attackFrames, 0.06f, true));
    m_leftHandAnimator.play("Idle");
    m_rightHandAnimator.play("Idle");
    configureLaser();
}

void SkelBoss::configureLaser() {
    auto &resources = ResourceManager::getInstance();
    const sf::Texture *texture = resources.getAtlasTexture("Boss");
    const auto *bodyFrames = resources.getAnimationFrames("Boss", "SkellBoss_WeaponLaserBody");
    const auto *headFrames = resources.getAnimationFrames("Boss", "SkellBoss_WeaponLaserHead");
    if (!texture || !bodyFrames || bodyFrames->empty() || !headFrames || headFrames->empty()) {
        return;
    }

    m_laserBody.emplace(*texture);
    m_laserHead.emplace(*texture);
    m_laserBody->setTextureRect(bodyFrames->front());
    m_laserHead->setTextureRect(headFrames->front());
    m_laserBodyAnimator.addAnimation("Fire", AnimationClip(bodyFrames, 0.06f, true));
    m_laserHeadAnimator.addAnimation("Fire", AnimationClip(headFrames, 0.06f, true));
    m_laserBodyAnimator.play("Fire");
    m_laserHeadAnimator.play("Fire");
}

sf::Vector2f SkelBoss::normalized(const sf::Vector2f &vector) {
    const float length = std::sqrt(vector.x * vector.x + vector.y * vector.y);
    return length > 0.001f ? vector / length : sf::Vector2f{0.f, 1.f};
}

float SkelBoss::directionAngle(const sf::Vector2f &direction) { return std::atan2(direction.y, direction.x); }

void SkelBoss::updateHandPositions(const TileMap &tileMap) {
    const sf::Vector2f mapSize = tileMap.getPixelSize();
    const sf::Vector2f tileSize = tileMap.getTileSize();
    const float horizontalInset = tileSize.x * kHandEdgeInsetTiles;
    m_leftHandPosition = {horizontalInset, m_leftHandY};
    m_rightHandPosition = {mapSize.x - horizontalInset, m_rightHandY};
    if (m_leftHand) {
        m_leftHand->setPosition(m_leftHandPosition);
    }
    if (m_rightHand) {
        m_rightHand->setPosition(m_rightHandPosition);
    }
}

void SkelBoss::updateHands(float dt, const TileMap &tileMap) {
    if (!m_leftHand || !m_rightHand) {
        return;
    }
    m_leftHandAnimator.update(dt, *m_leftHand);
    m_rightHandAnimator.update(dt, *m_rightHand);
    if (m_laserBody) {
        m_laserBodyAnimator.update(dt, *m_laserBody);
    }
    if (m_laserHead) {
        m_laserHeadAnimator.update(dt, *m_laserHead);
    }
    const sf::FloatRect leftBounds = m_leftHand->getLocalBounds();
    const sf::FloatRect rightBounds = m_rightHand->getLocalBounds();
    m_leftHand->setOrigin({leftBounds.size.x * 0.5f, leftBounds.size.y * 0.5f});
    m_rightHand->setOrigin({rightBounds.size.x * 0.5f, rightBounds.size.y * 0.5f});
    m_leftHand->setScale({kBossScale, kBossScale});
    // 오른손 전용 프레임이 없으므로 왼손 프레임을 X축 반전합니다.
    m_rightHand->setScale({-kBossScale, kBossScale});

    const sf::Vector2f mapSize = tileMap.getPixelSize();
    const sf::Vector2f tileSize = tileMap.getTileSize();
    const float minY = tileSize.y * 3.f;
    const float maxY = mapSize.y - tileSize.y * 3.f;
    const float baseY = std::clamp(getBodyCenterPosition().y, minY, maxY);
    m_handMotionTime += dt;
    const bool leftIsLaserHand = m_state == State::Attacking && m_currentPattern == SkelBossPattern::HandLaser && !m_attackingWithRightHand;
    const bool rightIsLaserHand = m_state == State::Attacking && m_currentPattern == SkelBossPattern::HandLaser && m_attackingWithRightHand;
    const float idleLeftY = baseY + std::sin(m_handMotionTime * 1.7f) * kHandIdleAmplitude;
    const float idleRightY = baseY + std::sin(m_handMotionTime * 1.7f + kPi) * kHandIdleAmplitude;
    const auto moveToward = [dt](float current, float target) {
        const float distance = target - current;
        const float maxStep = kHandMoveSpeed * dt;
        return std::abs(distance) <= maxStep ? target : current + (distance > 0.f ? maxStep : -maxStep);
    };
    if (m_leftHandY == 0.f) {
        m_leftHandY = idleLeftY;
    }
    if (m_rightHandY == 0.f) {
        m_rightHandY = idleRightY;
    }
    m_lockedHandY = std::clamp(m_lockedHandY, minY, maxY);
    m_leftHandY = leftIsLaserHand ? moveToward(m_leftHandY, m_lockedHandY) : moveToward(m_leftHandY, idleLeftY);
    m_rightHandY = rightIsLaserHand ? moveToward(m_rightHandY, m_lockedHandY) : moveToward(m_rightHandY, idleRightY);
    m_leftHandY = std::clamp(m_leftHandY, minY, maxY);
    m_rightHandY = std::clamp(m_rightHandY, minY, maxY);
    updateHandPositions(tileMap);
}

void SkelBoss::setHandAttackAnimation(bool rightHand) {
    if (rightHand) {
        m_leftHandAnimator.play("Idle");
        m_rightHandAnimator.play("Attack");
    } else {
        m_leftHandAnimator.play("Attack");
        m_rightHandAnimator.play("Idle");
    }
}

SkelBossPattern SkelBoss::choosePattern(const Player &player, const TileMap &tileMap) const {
    const sf::Vector2f playerCenter = player.getBodyCenterPosition();
    const float left = std::min(m_leftHandPosition.x, m_rightHandPosition.x);
    const float right = std::max(m_leftHandPosition.x, m_rightHandPosition.x);
    const bool isBetweenHands = playerCenter.x >= left && playerCenter.x <= right;
    const bool isNearFloor = playerCenter.y >= tileMap.getPixelSize().y * 0.65f;

    std::array<SkelBossPattern, 3> availablePatterns{};
    std::size_t availableCount = 0;
    if (isBetweenHands) {
        availablePatterns[availableCount++] = SkelBossPattern::HandLaser;
    }
    if (isNearFloor) {
        availablePatterns[availableCount++] = SkelBossPattern::SwordFan;
    }
    availablePatterns[availableCount++] = SkelBossPattern::RotatingBullet;

    const auto oldestIt = std::min_element(availablePatterns.begin(), availablePatterns.begin() + availableCount, [this](SkelBossPattern leftPattern, SkelBossPattern rightPattern) { return m_patternLastUsed[patternIndex(leftPattern)] < m_patternLastUsed[patternIndex(rightPattern)]; });
    return *oldestIt;
}

void SkelBoss::startPattern(SkelBossPattern pattern, const Player &player, ObjectPoolingManager &objectPool) {
    m_state = State::Attacking;
    m_currentPattern = pattern;
    m_patternTimer = 0.f;
    markPatternUsed(pattern);
    animator.play("SkellBoss_Attack");

    switch (pattern) {
    case SkelBossPattern::HandLaser:
        m_completedLaserHands = 0;
        beginLaserHand(player, false);
        break;
    case SkelBossPattern::RotatingBullet:
        m_bulletIntervalTimer = 0.f;
        m_bulletRotation = 0.f;
        break;
    case SkelBossPattern::SwordFan:
        std::cout << "[SkelBoss] SwordFan pattern started\n";
        summonSwordFan(objectPool);
        break;
    case SkelBossPattern::None:
        finishPattern();
        break;
    }
}

void SkelBoss::finishPattern() {
    m_currentPattern = SkelBossPattern::None;
    m_state = State::Idle;
    m_stateTimer = 0.f;
    m_laserActive = false;
    m_swords.clear();
    m_pendingSwordSpawns.clear();
    m_swordsLaunched = false;
    m_swordsReadyForAim = false;
    m_swordAimTimer = 0.f;
    m_leftHandAnimator.play("Idle");
    m_rightHandAnimator.play("Idle");
    animator.play("SkellBoss_Idle");
}

void SkelBoss::beginLaserHand(const Player &player, bool rightHand) {
    m_attackingWithRightHand = rightHand;
    m_patternTimer = 0.f;
    m_lockedLaserTarget = player.getBodyCenterPosition();
    m_lockedHandY = m_lockedLaserTarget.y;
    m_laserActive = false;
    m_laserDamageConsumed = false;
    setHandAttackAnimation(rightHand);
}

void SkelBoss::updateHandLaser(float dt, Player &player) {
    const sf::Vector2f origin = m_attackingWithRightHand ? m_rightHandPosition : m_leftHandPosition;
    const sf::Vector2f direction = m_attackingWithRightHand ? sf::Vector2f{-1.f, 0.f} : sf::Vector2f{1.f, 0.f};
    const float roomRight = m_leftHandPosition.x + m_rightHandPosition.x;
    const sf::Vector2f endPosition = m_attackingWithRightHand ? sf::Vector2f{0.f, origin.y} : sf::Vector2f{roomRight, origin.y};
    const float length = std::max(1.f, std::abs(endPosition.x - origin.x));
    m_laserBeam.setPosition(origin);
    m_laserBeam.setSize({length, kLaserWidth});
    m_laserBeam.setOrigin({0.f, kLaserWidth * 0.5f});
    const float angle = directionAngle(direction);
    m_laserBeam.setRotation(sf::radians(angle));

    if (m_laserBody) {
        const sf::FloatRect bounds = m_laserBody->getLocalBounds();
        m_laserBody->setOrigin({0.f, bounds.size.y * 0.5f});
        m_laserBody->setPosition(origin);
        m_laserBody->setScale({length / std::max(1.f, bounds.size.x), kBossScale});
        m_laserBody->setRotation(sf::radians(angle));
    }
    if (m_laserHead) {
        m_laserHead->setOrigin(m_laserHead->getLocalBounds().getCenter());
        m_laserHead->setPosition(endPosition);
        m_laserHead->setScale({kBossScale, kBossScale});
        m_laserHead->setRotation(sf::radians(angle));
    }

    const float handY = m_attackingWithRightHand ? m_rightHandY : m_leftHandY;
    if (std::abs(handY - m_lockedHandY) > 1.f) {
        m_laserActive = false;
        return;
    }

    m_patternTimer += dt;

    m_laserActive = m_patternTimer >= kLaserTelegraphDuration && m_patternTimer < kLaserTelegraphDuration + kLaserFireDuration;
    if (m_laserActive && !m_laserDamageConsumed && !player.dead() && !player.isDashing() && m_laserBeam.getGlobalBounds().findIntersection(player.getGlobalBounds())) {
        const float damage = m_handLaserWeapon ? m_handLaserWeapon->getStat().damage : 0.f;
        player.takeDamage(damage, origin, 0.25f);
        m_laserDamageConsumed = true;
    }

    if (m_patternTimer < kLaserTelegraphDuration + kLaserFireDuration) {
        return;
    }
    ++m_completedLaserHands;
    if (m_completedLaserHands >= 2) {
        finishPattern();
        return;
    }
    beginLaserHand(player, true);
}

sf::Vector2f SkelBoss::getMouthPosition() const {
    // 보스 스프라이트의 중심이 아니라, 해골 머리의 입이 위치한 높이를
    // 기준으로 발사점을 잡습니다. 프레임별 높이 변화에도 함께 보정됩니다.
    const sf::FloatRect bounds = getGlobalBounds();
    return {bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.66f};
}

std::size_t SkelBoss::patternIndex(SkelBossPattern pattern) {
    switch (pattern) {
    case SkelBossPattern::HandLaser:
        return 0;
    case SkelBossPattern::SwordFan:
        return 1;
    case SkelBossPattern::RotatingBullet:
        return 2;
    case SkelBossPattern::None:
        break;
    }
    return 0;
}

void SkelBoss::markPatternUsed(SkelBossPattern pattern) {
    if (pattern != SkelBossPattern::None) {
        m_patternLastUsed[patternIndex(pattern)] = ++m_patternUseSequence;
    }
}

void SkelBoss::fireRotatingCross(ObjectPoolingManager &objectPool) {
    if (!m_bulletWeapon) {
        std::cerr << "[SkelBoss] Bullet pattern skipped: bullet weapon is unavailable\n";
        return;
    }

    if (!m_bulletWeapon->getStat().projectile) {
        std::cerr << "[SkelBoss] Bullet pattern skipped: bullet projectile config is unavailable\n";
        return;
    }

    const sf::Vector2f origin = getMouthPosition();
    m_bulletWeapon->update(0.f, origin, m_bulletRotation);
    m_bulletWeapon->attack();
    const std::vector<ProjectileSpawnRequest> &requests = m_bulletWeapon->consumeProjectileRequests();
    if (requests.empty()) {
        std::cerr << "[SkelBoss] Bullet pattern produced no projectile requests\n";
        return;
    }

    objectPool.acquireEffect({"Projectile", "BossBulletFX", origin, 0.f, {1.f, 1.f}, 0.045f, false, false, 0.f, origin, sf::Color::White});
    unsigned int spawnedCount = 0;
    for (const ProjectileSpawnRequest &request : requests) {
        if (Projectile *bullet = objectPool.acquireProjectile(request)) {
            ++spawnedCount;
        }
    }
    std::cout << "[SkelBoss] BulletCross requested=" << requests.size() << ", spawned=" << spawnedCount << ", animation=" << requests.front().animationKey << '\n';
}

void SkelBoss::updateRotatingBullets(float dt, ObjectPoolingManager &objectPool) {
    m_patternTimer += dt;
    if (!m_bulletWeapon) {
        finishPattern();
        return;
    }

    const float fireInterval = std::max(kMinimumBulletInterval, m_bulletWeapon->getStat().attackSpeed);
    m_bulletIntervalTimer += dt;
    while (m_bulletIntervalTimer >= fireInterval) {
        m_bulletIntervalTimer -= fireInterval;
        fireRotatingCross(objectPool);
        // 발사 간격이 짧아져도 초당 회전 속도는 일정하게 유지합니다.
        m_bulletRotation += kBulletRotationSpeed * fireInterval;
    }
    if (m_patternTimer >= kBulletPatternDuration) {
        finishPattern();
    }
}

void SkelBoss::summonSwordFan(ObjectPoolingManager &objectPool) {
    if (!m_swordWeapon || !m_swordWeapon->getStat().projectile) {
        std::cerr << "[SkelBoss] SwordFan skipped: sword projectile config is unavailable\n";
        return;
    }

    const ProjectileConfig &config = *m_swordWeapon->getStat().projectile;
    const unsigned int swordCount = std::max(1u, config.count);
    stopSwordChargeEffects(objectPool);
    m_swords.clear();
    m_swords.reserve(swordCount);
    m_swordChargeEffects.reserve(swordCount);
    m_pendingSwordSpawns.clear();
    m_pendingSwordSpawns.reserve(swordCount);
    m_swordsLaunched = false;
    m_swordsReadyForAim = false;
    m_swordAimTimer = 0.f;
    const sf::Vector2f center = getBodyCenterPosition();
    for (unsigned int index = 0; index < swordCount; ++index) {
        const float ratio = swordCount == 1 ? 0.5f : static_cast<float>(index) / static_cast<float>(swordCount - 1);
        const float fanAngle = -kSwordFanHalfRadian + ratio * (kSwordFanHalfRadian * 2.f);
        const sf::Vector2f position = center + sf::Vector2f{std::sin(fanAngle) * 105.f, -82.f - std::cos(fanAngle) * 28.f};

        m_pendingSwordSpawns.push_back({position, kSwordSummonFxDuration});
        objectPool.acquireEffect({"Projectile", "BossSwordFX.png", position, 0.f, {1.f, 1.f}, 0.15f, false, false, 0.f, getBodyCenterPosition(), sf::Color(255, 245, 210)});
    }
}

void SkelBoss::spawnSword(const sf::Vector2f &position, ObjectPoolingManager &objectPool) {
    if (!m_swordWeapon) {
        std::cerr << "[SkelBoss] SwordFan skipped: sword weapon is unavailable\n";
        return;
    }
    const EquipStat swordStat = m_swordWeapon->getStat();
    if (!swordStat.projectile) {
        std::cerr << "[SkelBoss] SwordFan skipped: sword projectile config is unavailable\n";
        return;
    }
    m_swordWeapon->update(0.f, position, kPi * 0.5f);
    m_swordWeapon->attack();
    const std::vector<ProjectileSpawnRequest> &requests = m_swordWeapon->consumeProjectileRequests(1);
    if (requests.empty()) {
        std::cerr << "[SkelBoss] SwordFan produced no projectile request\n";
        return;
    }

    ProjectileSpawnRequest request = requests.front();
    request.position = position;
    request.direction = {0.f, 1.f};
    request.speed = 0.f;
    request.damageActiveOnSpawn = false;
    if (Projectile *sword = objectPool.acquireProjectile(request)) {
        m_swords.push_back(sword);
        std::cout << "[SkelBoss] SwordFan sword spawned: active=" << sword->isActive() << '\n';
        m_swordChargeEffects.push_back(objectPool.acquireEffect({"Projectile", "BossSword_Charge", position, 0.f, {1.f, 1.f}, 0.06f, true, false, 0.f, getBodyCenterPosition(), sf::Color::White}));
    }
}

void SkelBoss::stopSwordChargeEffects(ObjectPoolingManager &objectPool) {
    for (Effect *effect : m_swordChargeEffects) {
        objectPool.releaseEffect(effect);
    }
    m_swordChargeEffects.clear();
}

void SkelBoss::updateSwordFan(float dt, Player &player, ObjectPoolingManager &objectPool, EffectManager & /*effectManager*/, const TileMap & /*tileMap*/) {
    m_patternTimer += dt;
    if (!m_swordsLaunched) {
        for (auto spawnIt = m_pendingSwordSpawns.begin(); spawnIt != m_pendingSwordSpawns.end();) {
            spawnIt->remainingFxTime -= dt;
            if (spawnIt->remainingFxTime > 0.f) {
                ++spawnIt;
                continue;
            }
            spawnSword(spawnIt->position, objectPool);
            spawnIt = m_pendingSwordSpawns.erase(spawnIt);
        }
        if (!m_pendingSwordSpawns.empty()) {
            return;
        }
        if (!m_swordsReadyForAim) {
            m_swordsReadyForAim = true;
            m_swordAimTimer = 0.f;
        }
        m_swordAimTimer += dt;
        for (std::size_t index = 0; index < m_swords.size(); ++index) {
            Projectile *sword = m_swords[index];
            if (sword && sword->isActive()) {
                const sf::Vector2f direction = normalized(player.getBodyCenterPosition() - sword->getPosition());
                sword->setDirection(direction, 0.f);
                if (index < m_swordChargeEffects.size() && m_swordChargeEffects[index]) {
                    m_swordChargeEffects[index]->setPosition(sword->getPosition());
                    m_swordChargeEffects[index]->setRotationRadian(sword->getRotationRadian());
                }
            }
        }
        if (m_swordAimTimer >= kSwordAimDuration) {
            m_swordsLaunched = true;
            stopSwordChargeEffects(objectPool);
            float launchSpeed = 0.f;
            if (m_swordWeapon) {
                const EquipStat swordStat = m_swordWeapon->getStat();
                if (swordStat.projectile) {
                    launchSpeed = swordStat.projectile->speed;
                }
            }
            for (Projectile *sword : m_swords) {
                if (sword && sword->isActive()) {
                    sword->setDamageEnabled(true);
                    sword->setDirection(sword->getDirection(), launchSpeed);
                }
            }
        }
        return;
    }

    bool allImpacted = true;
    for (Projectile *sword : m_swords) {
        if (sword && sword->isActive() && !sword->isEmbedded()) {
            allImpacted = false;
        }
    }

    if (allImpacted || m_patternTimer >= kSwordSummonFxDuration + kSwordAimDuration + 4.f) {
        finishPattern();
    }
}

void SkelBoss::update(float dt, Player &player, ObjectPoolingManager &objectPool, EffectManager &effectManager, const TileMap &tileMap) {
    updateBossBase(dt);
    updateBackVisual(dt);
    updateHands(dt, tileMap);

    if (dead()) {
        m_state = State::Dead;
        m_laserActive = false;
        stopSwordChargeEffects(objectPool);
        return;
    }
    updateBackParticles(dt, objectPool);
    if (isSummoning()) {
        m_state = State::Summoning;
        return;
    }
    if (m_state == State::Summoning) {
        m_state = State::Idle;
        m_stateTimer = 0.f;
    }

    if (m_state == State::Idle) {
        m_stateTimer += dt;
        if (m_stateTimer >= kPostPatternIdleDuration) {
            startPattern(choosePattern(player, tileMap), player, objectPool);
        }
        return;
    }

    switch (m_currentPattern) {
    case SkelBossPattern::HandLaser:
        updateHandLaser(dt, player);
        break;
    case SkelBossPattern::RotatingBullet:
        updateRotatingBullets(dt, objectPool);
        break;
    case SkelBossPattern::SwordFan:
        updateSwordFan(dt, player, objectPool, effectManager, tileMap);
        break;
    case SkelBossPattern::None:
        finishPattern();
        break;
    }
}

void SkelBoss::render(sf::RenderWindow &window) {
    if (m_backSprite) {
        window.draw(*m_backSprite);
    }
    Actor::render(window);
    if (m_leftHand) {
        window.draw(*m_leftHand);
    }
    if (m_rightHand) {
        window.draw(*m_rightHand);
    }
    if (m_laserActive) {
        if (m_laserBody) {
            window.draw(*m_laserBody);
        }
        if (m_laserHead) {
            window.draw(*m_laserHead);
        }
    }
}
