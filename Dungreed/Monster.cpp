#include "Monster.h"

#include "Player.h"
#include "ResourceManager.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <iostream>

namespace {
const char* toString(MonsterState state) {
    switch (state) {
    case MonsterState::Idle: return "Idle";
    case MonsterState::Patrol: return "Patrol";
    case MonsterState::Chase: return "Chase";
    case MonsterState::Charge: return "Charge";
    case MonsterState::Attack: return "Attack";
    case MonsterState::Dead: return "Dead";
    }
    return "Unknown";
}
}

void Monster::init(const std::string& atlasKey) {
    if (m_isFlying) {
        movement.gravity = 0.f;
        movement.velocity = { 0.f, 0.f };
        movement.isGrounded = false;
    }
    Actor::init(atlasKey);

    auto& resources = ResourceManager::getInstance();
    const std::vector<std::string> animationNames = resources.getAnimationNames(atlasKey);
    const std::vector<sf::IntRect>* fallbackDeathFrames = nullptr;

    for (const std::string& animationName : animationNames) {
        const auto* frames = resources.getAnimationFrames(atlasKey, animationName);
        if (!frames) {
            continue;
        }
        if (!fallbackDeathFrames && animationName.size() >= 4 &&
            animationName.compare(animationName.size() - 4, 4, "_Die") == 0) {
            fallbackDeathFrames = frames;
        }
        if (animationName.find(m_type) == std::string::npos && animationName != "Monster_Die") {
            continue;
        }

        const bool isLoop = animationName.find("Attack") == std::string::npos &&
            animationName.find("Charge") == std::string::npos &&
            animationName.find("Die") == std::string::npos;
        float frameDuration = 0.2f;
        if (animationName.find("Attack") != std::string::npos ||
            animationName.find("Charge") != std::string::npos) {
            frameDuration = 0.1f;
        }
        animator.addAnimation(animationName, AnimationClip(frames, frameDuration, isLoop));
    }

    if (!animator.hasAnimation("Monster_Die") && fallbackDeathFrames) {
        animator.addAnimation("Monster_Die", AnimationClip(fallbackDeathFrames, 0.2f, false));
    }

    const auto* typeDeathFrames = resources.getAnimationFrames(atlasKey, m_type + "_Die");
    m_preferGenericDeathAnimation = typeDeathFrames && typeDeathFrames->size() <= 1 &&
        animator.hasAnimation("Monster_Die");

    if (animator.hasAnimation(m_type + "_Idle")) {
        animator.play(m_type + "_Idle");
    } else if (animator.hasAnimation(m_type + "_Charge")) {
        animator.play(m_type + "_Charge");
    } else if (animator.hasAnimation(m_type + "_Attack")) {
        animator.play(m_type + "_Attack");
    }
    updateAnimation(0.f);
}

void Monster::resetForReuse(Status newStatus, MonsterBehaviorConfig behavior) {
    Actor::resetForReuse(newStatus);
    state = MonsterState::Idle;
    fsm = MonsterFSMData{};
    setBehavior(behavior);
    m_attackCooldown = 0.f;
    m_attackActionConsumed = false;
    m_attackActionReady = false;
    m_chargeImpactConsumed = false;
    m_chargeDirection = 1.f;
    m_spawnActivationTimer = 0.f;
    m_spawnEffectDuration = 0.f;
    if (m_isFlying) {
        movement.gravity = 0.f;
        movement.velocity = { 0.f, 0.f };
        movement.isGrounded = false;
    }
}

void Monster::resetForReuse(const std::string& type, Status newStatus,
    const std::string& atlasKey, MonsterBehaviorConfig behavior) {
    m_type = type;
    resetForReuse(newStatus, behavior);
    init(atlasKey);
}

void Monster::setBehavior(MonsterBehaviorConfig behavior) {
    m_behavior = behavior;
    fsm.detectRange = behavior.detectRange;
    fsm.attackRange = behavior.attackRange;
    movement.moveSpeed = behavior.moveSpeed;
    m_isFlying = behavior.isFlying;
}

bool Monster::isTargetInAttackRange(const sf::Vector2f& targetPosition) const {
    const sf::Vector2f delta = targetPosition - getBodyCenterPosition();
    return delta.x * delta.x + delta.y * delta.y <= fsm.attackRange * fsm.attackRange;
}

void Monster::beginAttack() {
    changeState(MonsterState::Attack);
}

void Monster::beginSpawn(float duration) {
    m_spawnEffectDuration = std::max(0.f, duration);
    m_spawnActivationTimer = m_spawnEffectDuration;
    setHorizontalInput(0.f);
    if (m_isFlying) {
        movement.velocity.y = 0.f;
    }
}

bool Monster::canStartAttack() const {
    return m_attackCooldown <= 0.f;
}

void Monster::startAttackCooldown() {
    const auto weapon = getEquipment();
    const float attackSpeed = weapon ? weapon->getStat().attackSpeed : 1.f;
    m_attackCooldown = 1.f / std::max(attackSpeed, 0.01f);
    m_attackActionConsumed = false;
    m_attackActionReady = false;
}

void Monster::updateAttackCooldown(float dt) {
    m_attackCooldown = std::max(0.f, m_attackCooldown - dt);
}

bool Monster::consumeAttackAction() {
    if (!m_attackActionReady || m_attackActionConsumed) {
        return false;
    }
    m_attackActionConsumed = true;
    return true;
}

bool Monster::consumeChargeImpact() {
    if (!isChargeImpactActive() || m_chargeImpactConsumed) {
        return false;
    }
    m_chargeImpactConsumed = true;
    return true;
}

bool Monster::isChargeImpactActive() const {
    return state == MonsterState::Charge &&
        fsm.stateTimer >= m_behavior.attackWindup &&
        fsm.stateTimer < m_behavior.attackWindup + m_behavior.chargeDuration;
}

bool Monster::readyForPoolRelease() const {
    return dead() && state == MonsterState::Dead && animator.isFinished();
}

void Monster::changeState(MonsterState newState) {
    if (state == MonsterState::Dead) {
        return;
    }
    if (state != newState) {
        std::cout << "[MonsterFSM] type=" << m_type
                  << ", id=" << getId()
                  << ", " << toString(state)
                  << " -> " << toString(newState) << '\n';
    }

    state = newState;
    fsm.stateTimer = 0.f;
    m_attackActionReady = false;

    switch (state) {
    case MonsterState::Idle:
        setHorizontalInput(0.f);
        if (m_isFlying) {
            movement.velocity.y = 0.f;
        }
        if (animator.hasAnimation(m_type + "_Idle")) {
            animator.play(m_type + "_Idle");
        } else if (animator.hasAnimation(m_type + "_Charge")) {
            animator.play(m_type + "_Charge");
        } else if (animator.hasAnimation(m_type + "_Attack")) {
            animator.play(m_type + "_Attack");
        }
        break;

    case MonsterState::Patrol:
        fsm.patrolDirection = (std::rand() % 2 == 0) ? 1.f : -1.f;
        fsm.patrolVerticalDirection = (std::rand() % 2 == 0) ? 1.f : -1.f;
        if (animator.hasAnimation(m_type + "_Run")) {
            animator.play(m_type + "_Run");
        } else if (animator.hasAnimation(m_type + "_Idle")) {
            animator.play(m_type + "_Idle");
        }
        break;

    case MonsterState::Chase:
        if (animator.hasAnimation(m_type + "_Run")) {
            animator.play(m_type + "_Run");
        } else if (animator.hasAnimation(m_type + "_Idle")) {
            animator.play(m_type + "_Idle");
        }
        break;

    case MonsterState::Charge:
        setHorizontalInput(0.f);
        movement.velocity.y = 0.f;
        m_chargeDirection = fsm.facingDirection;
        m_chargeImpactConsumed = false;
        if (animator.hasAnimation(m_type + "_Charge")) {
            animator.play(m_type + "_Charge");
        } else if (animator.hasAnimation(m_type + "_Run")) {
            animator.play(m_type + "_Run");
        }
        break;

    case MonsterState::Attack:
        setHorizontalInput(0.f);
        if (m_isFlying) {
            movement.velocity.y = 0.f;
        }
        startAttackCooldown();
        if (animator.hasAnimation(m_type + "_Attack")) {
            animator.play(m_type + "_Attack");
        } else if (animator.hasAnimation(m_type + "_Charge")) {
            animator.play(m_type + "_Charge");
        } else if (animator.hasAnimation(m_type + "_Idle")) {
            animator.play(m_type + "_Idle");
        }
        break;

    case MonsterState::Dead:
        setHorizontalInput(0.f);
        movement.velocity.y = 0.f;
        m_attackActionReady = false;
        if (m_preferGenericDeathAnimation && animator.hasAnimation("Monster_Die")) {
            animator.play("Monster_Die");
        } else if (animator.hasAnimation(m_type + "_Die")) {
            animator.play(m_type + "_Die");
        } else if (animator.hasAnimation("Monster_Die")) {
            animator.play("Monster_Die");
        } else {
            animator.stop();
        }
        break;
    }
}

void Monster::handleFSM(float dt, const Player& player) {
    if (status.tmpHp <= 0.f && state != MonsterState::Dead) {
        changeState(MonsterState::Dead);
        return;
    }
    if (state == MonsterState::Dead) {
        return;
    }
    if (player.dead()) {
        if (state == MonsterState::Chase || state == MonsterState::Charge ||
            state == MonsterState::Attack) {
            changeState(MonsterState::Idle);
        }
        return;
    }

    fsm.stateTimer += dt;
    const sf::Vector2f centerPosition = getCenterPosition();
    const sf::Vector2f targetPosition = player.getCenterPosition();
    const float dx = targetPosition.x - centerPosition.x;
    const float dy = targetPosition.y - centerPosition.y;
    const float distance = std::sqrt(dx * dx + dy * dy);
    constexpr float horizontalDirectionDeadZone = 4.f;
    if (std::abs(dx) > horizontalDirectionDeadZone) {
        fsm.facingDirection = dx > 0.f ? 1.f : -1.f;
    }
    const float directionX = fsm.facingDirection;

    const auto stopMovement = [this]() {
        setHorizontalInput(0.f);
        if (m_isFlying) {
            movement.velocity.y = 0.f;
        }
    };
    const auto moveTowardPlayer = [this, dx, dy, distance, directionX]() {
        if (m_isFlying && distance > 0.01f) {
            const float speedScale = movement.moveSpeed / distance;
            movement.velocity = { dx * speedScale, dy * speedScale };
        } else {
            setHorizontalInput(directionX);
        }
        if (sprite) {
            sprite->setScale({ directionX, 1.f });
        }
    };
    const bool usesChargeCombo =
        m_behavior.attackPattern == MonsterAttackPattern::ChargeCombo;

    switch (state) {
    case MonsterState::Idle:
        if (distance <= fsm.detectRange) {
            if (usesChargeCombo && canStartAttack()) {
                changeState(MonsterState::Charge);
            } else {
                changeState(MonsterState::Chase);
            }
        } else if (fsm.stateTimer > 2.f) {
            changeState(MonsterState::Patrol);
        }
        break;

    case MonsterState::Patrol:
        setHorizontalInput(fsm.patrolDirection * 0.2f);
        if (m_isFlying) {
            movement.velocity.y = fsm.patrolVerticalDirection * movement.moveSpeed * 0.2f;
        }
        if (sprite) {
            sprite->setScale({ fsm.patrolDirection, 1.f });
        }
        if (distance <= fsm.detectRange) {
            if (usesChargeCombo && canStartAttack()) {
                changeState(MonsterState::Charge);
            } else {
                changeState(MonsterState::Chase);
            }
        } else if (fsm.stateTimer > 1.f) {
            changeState(MonsterState::Idle);
        }
        break;

    case MonsterState::Chase:
        if (distance > fsm.detectRange * 1.5f) {
            changeState(MonsterState::Idle);
        } else if (usesChargeCombo && canStartAttack()) {
            stopMovement();
            changeState(MonsterState::Charge);
        } else if (!usesChargeCombo && distance <= fsm.attackRange) {
            stopMovement();
            if (canStartAttack()) {
                changeState(MonsterState::Attack);
            }
        } else {
            moveTowardPlayer();
        }
        break;

    case MonsterState::Charge:
        if (fsm.stateTimer < m_behavior.attackWindup) {
            stopMovement();
        } else if (fsm.stateTimer <
            m_behavior.attackWindup + m_behavior.chargeDuration) {
            movement.velocity.x = m_chargeDirection * movement.moveSpeed *
                m_behavior.chargeSpeedMultiplier;
            movement.velocity.y = 0.f;
            if (sprite) {
                sprite->setScale({ m_chargeDirection, 1.f });
            }
        } else {
            stopMovement();
            changeState(MonsterState::Attack);
        }
        break;

    case MonsterState::Attack:
        stopMovement();
        if (m_type == "Bat") {
            m_attackActionReady = true;
            if (fsm.stateTimer > 1.f) {
                changeState(MonsterState::Idle);
            }
            break;
        }

        if (m_type == "SkelDog") {
            if (fsm.stateTimer >= m_behavior.attackWindup) {
                setHorizontalInput(directionX * 1.5f);
                m_attackActionReady = true;
                if (sprite) {
                    sprite->setScale({ directionX, 1.f });
                }
            }
            if (fsm.stateTimer > m_behavior.attackWindup + 0.45f) {
                changeState(MonsterState::Idle);
            }
            break;
        }

        if (fsm.stateTimer >= m_behavior.attackWindup &&
            (!m_behavior.attackOnLastFrame || animator.isOnLastFrame())) {
            m_attackActionReady = true;
        }
        if ((animator.isFinished() && m_attackActionConsumed) ||
            fsm.stateTimer > std::max(1.5f, m_behavior.attackWindup + 1.f)) {
            changeState(MonsterState::Idle);
        }
        break;

    case MonsterState::Dead:
        break;
    }
}

void Monster::update(float dt, Player& player) {
    updateAttackCooldown(dt);
    if (dead()) {
        handleFSM(dt, player);
        Actor::update(dt);
        return;
    }
    if (m_spawnActivationTimer > 0.f) {
        m_spawnActivationTimer = std::max(0.f, m_spawnActivationTimer - dt);
        setHorizontalInput(0.f);
        if (m_isFlying) {
            movement.velocity.y = 0.f;
        }
        Actor::update(dt);
        return;
    }
    handleFSM(dt, player);
    Actor::update(dt);
}

void Monster::render(sf::RenderWindow& window) {
    Actor::render(window);
    if (!isSpawning()) {
        return;
    }

    const sf::FloatRect bounds = getGlobalBounds();
    const float elapsed = m_spawnEffectDuration - m_spawnActivationTimer;
    const float pulse = (std::sin(elapsed * 18.f) + 1.f) * 0.5f;
    const auto alpha = static_cast<std::uint8_t>(80.f + pulse * 100.f);
    sf::RectangleShape highlight(bounds.size);
    highlight.setPosition(bounds.position);
    highlight.setFillColor(sf::Color(255, 255, 255, alpha / 3));
    highlight.setOutlineColor(sf::Color(255, 255, 255, alpha));
    highlight.setOutlineThickness(2.f);
    window.draw(highlight);
}
