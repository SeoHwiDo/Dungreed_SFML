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

        const std::string typePrefix = m_type + "_";
        const std::string stateName = animationName.compare(0, typePrefix.size(), typePrefix) == 0
            ? animationName.substr(typePrefix.size())
            : "Die";
        const auto configIt = m_behavior.animations.find(stateName);
        const MonsterAnimationConfig animationConfig =
            configIt == m_behavior.animations.end()
            ? MonsterAnimationConfig{}
            : configIt->second;
        animator.addAnimation(animationName, AnimationClip(frames,
            animationConfig.frameDuration, animationConfig.isLoop));
    }

    if (!animator.hasAnimation("Monster_Die") && fallbackDeathFrames) {
        const auto deathConfigIt = m_behavior.animations.find("Die");
        const MonsterAnimationConfig deathConfig =
            deathConfigIt == m_behavior.animations.end()
            ? MonsterAnimationConfig{}
            : deathConfigIt->second;
        animator.addAnimation("Monster_Die", AnimationClip(fallbackDeathFrames,
            deathConfig.frameDuration, deathConfig.isLoop));
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
    m_attackPhase = MonsterAttackPhase::Active;
    m_attackFacingDirection = 1.f;
    m_attackWasInterruptedByHit = false;
    m_attackReleasePlayed = false;
    m_chargeImpactConsumed = false;
    m_chargeDirection = 1.f;
    m_spawnActivationTimer = 0.f;
    m_spawnEffectDuration = 0.f;
    if (m_isFlying) {
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
    movement.gravity = behavior.gravity;
    m_isFlying = behavior.isFlying;
}

bool Monster::isTargetInAttackRange(const sf::Vector2f& targetPosition) const {
    const sf::Vector2f delta = targetPosition - getBodyCenterPosition();
    return delta.x * delta.x + delta.y * delta.y <= fsm.attackRange * fsm.attackRange;
}

sf::FloatRect Monster::getFrontAttackBounds() const {
    sf::FloatRect frontBounds = getCollision().getHitbox();
    const float halfWidth = frontBounds.size.x * 0.5f;
    constexpr float frontAttackPadding = 3.f;
    const float facingDirection = m_behavior.lockAttackFacing &&
        state == MonsterState::Attack
        ? m_attackFacingDirection
        : fsm.facingDirection;
    // 월드 X축의 양의 방향은 오른쪽입니다. 오른쪽을 볼 때는 몸체의 오른쪽 절반을 사용합니다.
    if (facingDirection > 0.f) {
        frontBounds.position.x += halfWidth;
    } else {
        frontBounds.position.x -= frontAttackPadding;
    }
    frontBounds.size.x = halfWidth + frontAttackPadding;
    return frontBounds;
}

bool Monster::isTargetInFrontContact(const sf::FloatRect& targetBounds) const {
    return getFrontAttackBounds().findIntersection(targetBounds).has_value();
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

void Monster::playAttackAnimation() {
    if (animator.hasAnimation(m_type + "_Attack")) {
        animator.play(m_type + "_Attack");
    } else if (animator.hasAnimation(m_type + "_Charge")) {
        animator.play(m_type + "_Charge");
    } else if (animator.hasAnimation(m_type + "_Idle")) {
        animator.play(m_type + "_Idle");
    }
}

bool Monster::playAttackReleaseAnimation() {
    const std::string releaseAnimation = m_type + "_Release";
    if (!animator.hasAnimation(releaseAnimation)) {
        return false;
    }
    animator.play(releaseAnimation);
    return true;
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

float Monster::getChargeDistance() const {
    const float chargeSpeed = movement.moveSpeed * m_behavior.chargeSpeedMultiplier;
    return std::max(0.f, chargeSpeed * m_behavior.chargeDuration);
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

    const MonsterState previousState = state;
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

    case MonsterState::Attack: {
        setHorizontalInput(0.f);
        if (m_isFlying) {
            movement.velocity.y = 0.f;
        }
        m_attackFacingDirection = previousState == MonsterState::Charge
            ? m_chargeDirection
            : fsm.facingDirection;
        m_attackWasInterruptedByHit = false;
        m_attackReleasePlayed = false;
        if (m_behavior.lockAttackFacing && sprite) {
            sprite->setScale({ m_attackFacingDirection, 1.f });
        }
        startAttackCooldown();
        if (animator.hasAnimation(m_type + "_AttackReady")) {
            m_attackPhase = MonsterAttackPhase::Ready;
            animator.play(m_type + "_AttackReady");
        } else {
            m_attackPhase = MonsterAttackPhase::Active;
            playAttackAnimation();
            m_attackActionReady = true;
        }
        break;
    }

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
    const float horizontalDistance = std::abs(dx);

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
    const bool isWithinChargeDistance = horizontalDistance <= getChargeDistance();
    const auto weapon = getEquipment();
    const bool isMeleeAttack = weapon &&
        weapon->getStat().type == WeaponType::Melee;
    const bool isInMeleeAttackContact =
        isTargetInFrontContact(player.getGlobalBounds());

    switch (state) {
    case MonsterState::Idle:
        if (distance <= fsm.detectRange) {
            if (usesChargeCombo && canStartAttack() && isWithinChargeDistance) {
                changeState(MonsterState::Charge);
            } else {
                changeState(MonsterState::Chase);
            }
        } else if (fsm.stateTimer > m_behavior.idleDuration) {
            changeState(MonsterState::Patrol);
        }
        break;

    case MonsterState::Patrol:
        setHorizontalInput(fsm.patrolDirection * m_behavior.patrolSpeedMultiplier);
        if (m_isFlying) {
            movement.velocity.y = fsm.patrolVerticalDirection * movement.moveSpeed *
                m_behavior.patrolSpeedMultiplier;
        }
        if (sprite) {
            sprite->setScale({ fsm.patrolDirection, 1.f });
        }
        if (distance <= fsm.detectRange) {
            if (usesChargeCombo && canStartAttack() && isWithinChargeDistance) {
                changeState(MonsterState::Charge);
            } else {
                changeState(MonsterState::Chase);
            }
        } else if (fsm.stateTimer > m_behavior.patrolDuration) {
            changeState(MonsterState::Idle);
        }
        break;

    case MonsterState::Chase:
        if (distance > fsm.detectRange * m_behavior.chaseExitRangeMultiplier) {
            changeState(MonsterState::Idle);
        } else if (usesChargeCombo && canStartAttack() && isWithinChargeDistance) {
            stopMovement();
            changeState(MonsterState::Charge);
        } else if (!usesChargeCombo && distance <= fsm.attackRange &&
            (!isMeleeAttack || isInMeleeAttackContact)) {
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

    case MonsterState::Attack: {
        stopMovement();
        if (isHit()) {
            m_attackWasInterruptedByHit = true;
        }

        // AttackReady 프레임이 있는 몬스터만 JSON에 설정한 선딜 시간 동안 재생합니다.
        if (m_attackPhase == MonsterAttackPhase::Ready) {
            if (fsm.stateTimer >= m_behavior.attackReadyDuration) {
                m_attackPhase = MonsterAttackPhase::Active;
                fsm.stateTimer = 0.f;
                playAttackAnimation();
                m_attackActionReady = true;
            }
            break;
        }

        // 실제 판정 후에는 JSON에 설정한 회수/후딜 규칙을 적용합니다.
        if (m_attackPhase == MonsterAttackPhase::Recovery) {
            if (m_behavior.waitForAttackCooldownAfterRelease) {
                if (isHit()) {
                    m_attackWasInterruptedByHit = true;
                    if (m_attackReleasePlayed && animator.hasAnimation(m_type + "_Idle")) {
                        animator.play(m_type + "_Idle");
                    }
                    m_attackReleasePlayed = false;
                }

                const bool releaseFinished = !m_attackReleasePlayed || animator.isFinished();
                if (releaseFinished && m_attackCooldown <= 0.f) {
                    changeState(MonsterState::Idle);
                }
                break;
            }
            if (fsm.stateTimer >= m_behavior.attackRecoveryDuration) {
                changeState(MonsterState::Idle);
            }
            break;
        }

        if (m_attackActionConsumed || fsm.stateTimer > m_behavior.attackActiveTimeout) {
            m_attackPhase = MonsterAttackPhase::Recovery;
            m_attackActionReady = false;
            fsm.stateTimer = 0.f;
            stopMovement();
            if (m_behavior.playReleaseAnimation) {
                const bool canPlayRelease = !m_attackWasInterruptedByHit && !isHit();
                m_attackReleasePlayed = canPlayRelease && playAttackReleaseAnimation();
                if (!m_attackReleasePlayed && animator.hasAnimation(m_type + "_Idle")) {
                    animator.play(m_type + "_Idle");
                }
            }
        }
        break;
    }

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
