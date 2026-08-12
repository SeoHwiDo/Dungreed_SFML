#include "Player.h"

#include "Collision.h"
#include "Equip.h"
#include "ResourceManager.h"
#include "TileMap.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

void Player::init(const std::string &atlasKey) {
    Actor::init(atlasKey);
    if (!equipment) {
        auto defaultWeapon = std::make_shared<Equip>("ShortSword", EquipStat{10.f, 2.5f, 40.f});
        defaultWeapon->init("Equip", "ShortSword_Idle-00");
        setEquipment(defaultWeapon);
    }

    auto &resourceManager = ResourceManager::getInstance();
    for (const std::string &animationName : resourceManager.getAnimationNames(atlasKey)) {
        const auto *frames = resourceManager.getAnimationFrames(atlasKey, animationName);
        if (!frames) {
            continue;
        }

        const bool isLoop = animationName.find("Attack") == std::string::npos && animationName.find("Dead") == std::string::npos && animationName.find("Die") == std::string::npos;
        float frameDuration = 0.10f;
        // if (animationName.find("Idle") != std::string::npos) {
        //     frameDuration = 0.10f;
        // } else if (animationName.find("Run") != std::string::npos) {
        //     frameDuration = 0.10f;
        // } else if (animationName.find("Attack") != std::string::npos) {
        //     frameDuration = 0.10f;
        // }
        animator.addAnimation(animationName, AnimationClip(frames, frameDuration, isLoop));
    }
    animator.play("Player_Idle");
    prewarmDashAfterimages();
    // 첫 update 전에도 아틀라스 전체가 보이지 않도록 Idle의 첫 프레임을 즉시 적용합니다.
    updateAnimation(0.f);
}

void Player::changeState(PlayerState newState) {
    if (state == newState) {
        return;
    }

    state = newState;
    switch (state) {
    case PlayerState::Idle:
        setHorizontalInput(0.f);
        playAnimation("Player_Idle");
        break;
    case PlayerState::Run:
        playAnimation("Player_Run");
        break;
    case PlayerState::Jump:
        playAnimation("Player_Jump");
        break;
    case PlayerState::Dash:
        setHorizontalInput(0.f);
        playAnimation("Player_Run");
        break;
    case PlayerState::Dead:
        setHorizontalInput(0.f);
        movement.velocity.y = 0.f;
        m_knockbackVelocity = {0.f, 0.f};
        m_knockbackTimer = 0.f;
        playAnimation("Player_Die");
        break;
    }
}

void Player::setDashConfig(DashConfig config) {
    config.maxDistanceMultiplier = std::max(0.f, config.maxDistanceMultiplier);
    config.maxCharges = std::max(1, config.maxCharges);
    config.chargeRecoveryTime = std::max(0.01f, config.chargeRecoveryTime);
    config.duration = std::max(0.01f, config.duration);
    config.afterimageInterval = std::max(0.01f, config.afterimageInterval);
    config.afterimageLifetime = std::max(0.01f, config.afterimageLifetime);

    m_dashConfig = config;
    m_dashCharges = std::min(m_dashCharges, m_dashConfig.maxCharges);
    if (m_dashCharges >= m_dashConfig.maxCharges) {
        m_dashRechargeTimer = 0.f;
    }
}

float Player::getDashRechargeProgress() const {
    if (m_dashCharges >= m_dashConfig.maxCharges) {
        return 1.f;
    }
    return std::clamp(m_dashRechargeTimer / m_dashConfig.chargeRecoveryTime, 0.f, 1.f);
}

void Player::restoreDashCharges(int amount) {
    m_dashCharges = std::clamp(m_dashCharges + std::max(0, amount), 0, m_dashConfig.maxCharges);
    if (m_dashCharges >= m_dashConfig.maxCharges) {
        m_dashRechargeTimer = 0.f;
    }
    prewarmDashAfterimages();
}

void Player::restoreForVillage() {
    resetForReuse({kDefaultMaxHp, kDefaultMaxHp, kDefaultPower, kDefaultDex});
    m_dashCharges = m_dashConfig.maxCharges;
    m_dashRechargeTimer = 0.f;
    m_isDashing = false;
    m_ignoreOneWayPlatforms = false;
    m_dropThroughTimer = 0.f;
    m_dashElapsed = 0.f;
    m_afterimageTimer = 0.f;
    m_stunTimer = 0.f;
    for (DashAfterimage &afterimage : m_dashAfterimages) {
        afterimage.active = false;
        afterimage.remainingTime = 0.f;
    }
    changeState(PlayerState::Idle);
}

bool Player::tryStartDash(const sf::Vector2f &cursorPosition) {
    if (m_isDashing || m_dashCharges <= 0 || !sprite) {
        return false;
    }

    const sf::Vector2f toCursor = cursorPosition - getBodyCenterPosition();
    const float cursorDistance = std::sqrt(toCursor.x * toCursor.x + toCursor.y * toCursor.y);
    if (cursorDistance <= 0.01f) {
        return false;
    }

    const float maxDistance = getGlobalBounds().size.x * m_dashConfig.maxDistanceMultiplier;
    const float dashDistance = std::min(cursorDistance, maxDistance);
    m_dashDelta = toCursor * (dashDistance / cursorDistance);
    m_dashElapsed = 0.f;
    m_afterimageTimer = 0.f;
    m_isDashing = true;
    --m_dashCharges;
    changeState(PlayerState::Dash);
    spawnDashAfterimage();
    return true;
}

void Player::cancelDash() {
    if (!m_isDashing) {
        return;
    }

    m_isDashing = false;
    m_ignoreOneWayPlatforms = true;
    m_dashElapsed = m_dashConfig.duration;
    movement.velocity.x = 0.f;
    setHorizontalInput(0.f);
}

void Player::updateDash(float dt, const TileMap &tileMap) {
    m_previousGlobalBounds = getGlobalBounds();
    const float previousProgress = m_dashElapsed / m_dashConfig.duration;
    m_dashElapsed = std::min(m_dashElapsed + dt, m_dashConfig.duration);
    const float currentProgress = m_dashElapsed / m_dashConfig.duration;
    const sf::Vector2f delta = m_dashDelta * (currentProgress - previousProgress);

    const sf::Vector2f tileSize = tileMap.getTileSize();
    const float maximumStep = std::max(1.f, std::min(tileSize.x, tileSize.y) * 0.25f);
    const int stepCount = std::max(1, static_cast<int>(std::ceil(std::max(std::abs(delta.x), std::abs(delta.y)) / maximumStep)));
    const sf::Vector2f step = delta / static_cast<float>(stepCount);

    for (int index = 0; index < stepCount; ++index) {
        const sf::Vector2f positionBeforeStep = getPosition();
        move(step.x, step.y);
        Collision::resolveMapCollision(*this, tileMap, true);

        const sf::Vector2f actualMovement = getPosition() - positionBeforeStep;
        constexpr float collisionEpsilon = 0.1f;
        if (std::abs(actualMovement.x - step.x) > collisionEpsilon || std::abs(actualMovement.y - step.y) > collisionEpsilon) {
            cancelDash();
            break;
        }
    }

    m_afterimageTimer += dt;
    while (m_afterimageTimer >= m_dashConfig.afterimageInterval) {
        m_afterimageTimer -= m_dashConfig.afterimageInterval;
        spawnDashAfterimage();
    }

    if (m_dashElapsed >= m_dashConfig.duration) {
        cancelDash();
    }
}
void Player::updateDashRecharge(float dt) {
    if (m_dashCharges >= m_dashConfig.maxCharges) {
        m_dashRechargeTimer = 0.f;
        return;
    }

    m_dashRechargeTimer += dt;
    while (m_dashRechargeTimer >= m_dashConfig.chargeRecoveryTime && m_dashCharges < m_dashConfig.maxCharges) {
        m_dashRechargeTimer -= m_dashConfig.chargeRecoveryTime;
        ++m_dashCharges;
    }
}

void Player::spawnDashAfterimage() {
    if (!sprite) {
        return;
    }

    const auto inactiveIt = std::find_if(m_dashAfterimages.begin(), m_dashAfterimages.end(), [](const DashAfterimage &afterimage) { return !afterimage.active; });
    if (inactiveIt == m_dashAfterimages.end()) {
        return;
    }

    DashAfterimage &afterimage = *inactiveIt;
    afterimage.sprite = *sprite;
    afterimage.remainingTime = m_dashConfig.afterimageLifetime;
    afterimage.active = true;
    afterimage.sprite.setColor(sf::Color(190, 220, 255, 140));
}

void Player::updateAfterimages(float dt) {
    for (DashAfterimage &afterimage : m_dashAfterimages) {
        if (!afterimage.active) {
            continue;
        }
        afterimage.remainingTime -= dt;
        const float alphaRatio = std::clamp(afterimage.remainingTime / m_dashConfig.afterimageLifetime, 0.f, 1.f);
        sf::Color color = afterimage.sprite.getColor();
        color.a = static_cast<std::uint8_t>(140.f * alphaRatio);
        afterimage.sprite.setColor(color);
        if (afterimage.remainingTime <= 0.f) {
            afterimage.active = false;
        }
    }
}

void Player::prewarmDashAfterimages() {
    if (!sprite) {
        return;
    }
    const float requiredLifetime = std::max(m_dashConfig.duration, m_dashConfig.afterimageLifetime);
    const std::size_t requiredCount = static_cast<std::size_t>(std::ceil(requiredLifetime / m_dashConfig.afterimageInterval)) + 1;
    if (m_dashAfterimages.size() < requiredCount) {
        m_dashAfterimages.reserve(requiredCount);
        while (m_dashAfterimages.size() < requiredCount) {
            m_dashAfterimages.emplace_back(*sprite);
        }
    }
}

void Player::applyStun(float duration) {
    if (dead() || duration <= 0.f) {
        return;
    }

    m_stunTimer = std::max(m_stunTimer, duration);
    m_isDashing = false;
    m_ignoreOneWayPlatforms = false;
    m_dropThroughTimer = 0.f;
    movement.velocity.x = 0.f;
    setHorizontalInput(0.f);
}

void Player::updateFacingDirection(const sf::Vector2f &aimWorldPosition) {
    const float horizontalDistance = aimWorldPosition.x - getBodyCenterPosition().x;
    if (horizontalDistance == 0) {
        return;
    }

    m_facingDirection = horizontalDistance < 0 ? -1.f : 1.f;
    if (sprite) {
        sprite->setScale({m_facingDirection, 1.f});
    }
}
void Player::handleState(float dt, const InputData &input, const TileMap &tileMap) {
    if (dead()) {
        changeState(PlayerState::Dead);
        return;
    }

    if (input.isDashing && tryStartDash(input.aimWorldPosition)) {
        // 대시 시작과 동시에 장비 공격을 실행해, 이동 중에도 무기 판정을 유지합니다.
        if (equipment) {
            equipment->attack();
        }
        return;
    }
    if (m_isDashing) {
        // 대시 도중의 공격 입력도 무시하지 않습니다.
        if (input.isAttacking && equipment) {
            equipment->attack();
        }
        return;
    }

    if (input.isDroppingThrough && movement.isGrounded) {
        const sf::FloatRect bounds = getGlobalBounds();
        constexpr float groundEpsilon = 2.f;
        const sf::FloatRect footCheck({bounds.position.x + 2.f, bounds.position.y + bounds.size.y}, {std::max(0.f, bounds.size.x - 4.f), groundEpsilon});

        const bool standingOnOneWay = std::any_of(tileMap.getCollisionTiles().begin(), tileMap.getCollisionTiles().end(), [&footCheck](const TileData &tile) { return tile.type == TileType::OneWay && footCheck.findIntersection(tile.bounds).has_value(); });
        if (standingOnOneWay) {
            // 플랫폼 두께를 완전히 통과할 때까지 one-way 충돌을 잠시 무시합니다.
            m_dropThroughTimer = 0.20f;
            movement.isGrounded = false;
            movement.velocity.y = std::max(movement.velocity.y, 180.f);
        }
    }

    if (input.isJumping && movement.isGrounded) {
        jump();
    }

    if (!movement.isGrounded) {
        changeState(PlayerState::Jump);
    } else if (input.moveDirX != 0.f) {
        changeState(PlayerState::Run);
    } else {
        changeState(PlayerState::Idle);
    }

    setHorizontalInput(input.moveDirX);

    if (input.isAttacking && equipment) {
        equipment->attack();
    }
    if (equipment) {
        equipment->update(dt, getBodyCenterPosition(), input.aimRadian);
    }
}

void Player::update(float dt, const sf::RenderWindow &window, const TileMap &tileMap) {
    m_dropThroughTimer = std::max(0.f, m_dropThroughTimer - dt);
    if (!m_isDashing) {
        m_ignoreOneWayPlatforms = false;
    }
    if (dead()) {
        changeState(PlayerState::Dead);
        updatePhysics(dt);
        updateHitFeedback(dt);
        updateAnimation(dt);
        updateAfterimages(dt);
        return;
    }

    if (m_stunTimer > 0.f) {
        m_stunTimer = std::max(0.f, m_stunTimer - dt);
        m_isDashing = false;
        m_ignoreOneWayPlatforms = false;
        setHorizontalInput(0.f);
        if (movement.isGrounded) {
            changeState(PlayerState::Idle);
        }
        updatePhysics(dt);
        updateHitFeedback(dt);
        updateAnimation(dt);
        updateAfterimages(dt);
        return;
    }

    const InputData input = controller.getInput(window, getBodyCenterPosition());
    updateFacingDirection(input.aimWorldPosition);
    updateDashRecharge(dt);
    updateAfterimages(dt);
    handleState(dt, input, tileMap);

    if (m_isDashing) {
        updateDash(dt, tileMap);
        if (equipment) {
            equipment->update(dt, getBodyCenterPosition(), input.aimRadian);
        }
        updateHitFeedback(dt);
        updateAnimation(dt);
        return;
    }

    updatePhysics(dt);
    updateHitFeedback(dt);
    updateAnimation(dt);
}

void Player::render(sf::RenderWindow &window) {
    for (const DashAfterimage &afterimage : m_dashAfterimages) {
        if (afterimage.active) {
            window.draw(afterimage.sprite);
        }
    }
    Actor::render(window);
}

void Player::takeDamage(float damage, const sf::Vector2f &attackerPosition, float knockbackMultiplier) {
    if (m_isDashing) {
        return;
    }
    Actor::takeDamage(damage, attackerPosition, knockbackMultiplier);
}
