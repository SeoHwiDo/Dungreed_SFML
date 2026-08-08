#include "Monster.h"

#include "Player.h"
#include "ResourceManager.h"
#include <cmath>
#include <cstdlib>

void Monster::init(const std::string& atlasKey) {
    m_isFlying = (m_type == "Bat");
    if (m_isFlying) {
        movement.gravity = 0.f;
        movement.velocity = { 0.f, 0.f };
        movement.isGrounded = false;
    }
    Actor::init(atlasKey);

    auto& resMgr = ResourceManager::getInstance();
    const std::vector<std::string> allAnims = resMgr.getAnimationNames(atlasKey);
    const std::vector<sf::IntRect>* fallbackDieFrames = nullptr;

    for (const auto& animName : allAnims) {
        const auto* frames = resMgr.getAnimationFrames(atlasKey, animName);
        if (!frames) {
            continue;
        }

        if (!fallbackDieFrames && animName.size() >= 4 &&
            animName.compare(animName.size() - 4, 4, "_Die") == 0) {
            fallbackDieFrames = frames;
        }

        if (animName.find(m_type) == std::string::npos && animName != "Monster_Die") {
            continue;
        }

        const bool isLoop = animName.find("Attack") == std::string::npos &&
            animName.find("Charge") == std::string::npos &&
            animName.find("Die") == std::string::npos;
        animator.addAnimation(animName, AnimationClip(frames, 0.1f, isLoop));
    }

    if (!animator.hasAnimation("Monster_Die") && fallbackDieFrames) {
        animator.addAnimation("Monster_Die", AnimationClip(fallbackDieFrames, 0.1f, false));
    }
    animator.play(m_type + "_Idle");
}

void Monster::resetForReuse(Status newStatus, MonsterBehaviorConfig behavior) {
    Actor::resetForReuse(newStatus);
    state = MonsterState::Idle;
    fsm = MonsterFSMData{};
    setBehavior(behavior);
    m_attackCooldown = 0.f;
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
    fsm.DETECT_RANGE = behavior.detectRange;
    fsm.ATTACK_RANGE = behavior.attackRange;
    movement.moveSpeed = behavior.moveSpeed;
}

bool Monster::isTargetInAttackRange(const sf::Vector2f& targetPosition) const {
    const sf::Vector2f delta = targetPosition - getBodyCenterPosition();
    return delta.x * delta.x + delta.y * delta.y <= fsm.ATTACK_RANGE * fsm.ATTACK_RANGE;
}

void Monster::beginAttack() {
    changeState(MonsterState::Attack);
}

bool Monster::consumeAttackCooldown(float dt) {
    m_attackCooldown = std::max(0.f, m_attackCooldown - dt);
    if (m_attackCooldown > 0.f) {
        return false;
    }
    const auto weapon = getEquipment();
    const float attackSpeed = weapon ? weapon->getStat().attackSpeed : 1.f;
    m_attackCooldown = 1.f / std::max(attackSpeed, 0.01f);
    return true;
}

bool Monster::readyForPoolRelease() const {
    return dead() && state == MonsterState::Dead && animator.isFinished();
}

void Monster::changeState(MonsterState newState) {
    if (state == MonsterState::Dead) {
        return;
    }

    state = newState;
    fsm.m_stateTimer = 0.f;

    switch (state) {
    case MonsterState::Idle:
        setHorizontalInput(0.f);
        animator.play(m_type + "_Idle");
        break;

    case MonsterState::Patrol:
        fsm.m_patrolDir = (std::rand() % 2 == 0) ? 1.f : -1.f;
        if (animator.hasAnimation(m_type + "_Run")) {
            animator.play(m_type + "_Run");
        } else {
            animator.play(m_type + "_Idle");
        }
        break;

    case MonsterState::Chase:
        if (animator.hasAnimation(m_type + "_Run")) {
            animator.play(m_type + "_Run");
        } else {
            animator.play(m_type + "_Idle");
        }
        break;

    case MonsterState::Attack:
        setHorizontalInput(0.f);
        if (animator.hasAnimation(m_type + "_Attack")) {
            animator.play(m_type + "_Attack");
        } else if (animator.hasAnimation(m_type + "_Charge")) {
            animator.play(m_type + "_Charge");
        } else {
            animator.play(m_type + "_Idle");
        }
        break;

    case MonsterState::Dead:
        setHorizontalInput(0.f);
        if (animator.hasAnimation(m_type + "_Die")) {
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
    if (status.tmpHp <= 0 && state != MonsterState::Dead) {
        changeState(MonsterState::Dead);
        return;
    }
    if (state == MonsterState::Dead) {
        return;
    }

    if (player.dead()) {
        if (state == MonsterState::Chase || state == MonsterState::Attack) {
            changeState(MonsterState::Idle);
        }
        return;
    }

    fsm.m_stateTimer += dt;
    const sf::Vector2f centerPos = getCenterPosition();
    const sf::Vector2f targetCenter = player.getCenterPosition();
    const float dx = targetCenter.x - centerPos.x;
    const float dy = targetCenter.y - centerPos.y;
    const float distance = std::sqrt(dx * dx + dy * dy);
    const float directionX = dx >= 0.f ? 1.f : -1.f;

    switch (state) {
    case MonsterState::Idle:
        if (distance <= fsm.DETECT_RANGE) {
            changeState(MonsterState::Chase);
        } else if (fsm.m_stateTimer > 2.f) {
            changeState(MonsterState::Patrol);
        }
        break;

    case MonsterState::Patrol:
        setHorizontalInput(fsm.m_patrolDir * 0.2f);
        if (sprite) {
            sprite->setScale({ fsm.m_patrolDir, 1.f });
        }
        if (distance <= fsm.DETECT_RANGE) {
            changeState(MonsterState::Chase);
        } else if (fsm.m_stateTimer > 1.f) {
            changeState(MonsterState::Idle);
        }
        break;

    case MonsterState::Chase:
        if (distance <= fsm.ATTACK_RANGE) {
            changeState(MonsterState::Attack);
        } else if (distance > fsm.DETECT_RANGE * 1.5f) {
            changeState(MonsterState::Idle);
        } else {
            setHorizontalInput(directionX);
            if (sprite) {
                sprite->setScale({ directionX, 1.f });
            }
        }
        break;

    case MonsterState::Attack:
        if (animator.isFinished() || fsm.m_stateTimer > 1.f) {
            changeState(MonsterState::Idle);
        }
        break;

    case MonsterState::Dead:
        break;
    }
}

void Monster::update(float dt, Player& player) {
    handleFSM(dt, player);
    Actor::update(dt);
}