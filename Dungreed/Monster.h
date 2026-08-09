#pragma once

#include "Actor.h"

#include <SFML/System/Vector2.hpp>

#include <string>

enum class MonsterState {
    Idle,
    Patrol,
    Chase,
    Charge,
    Attack,
    Dead
};

enum class MonsterAttackPattern {
    Standard,
    Ranged,
    GhostTouch,
    RadialProjectile,
    ChargeCombo
};

struct MonsterFSMData {
    float stateTimer = 0.f;
    float patrolDirection = 1.f;
    float patrolVerticalDirection = 1.f;
    float facingDirection = 1.f;
    float detectRange = 100.f;
    float attackRange = 50.f;
};

struct MonsterBehaviorConfig {
    float detectRange = 100.f;
    float attackRange = 50.f;
    float moveSpeed = 300.f;
    float attackWindup = 0.35f;
    float chargeDuration = 0.55f;
    float chargeSpeedMultiplier = 3.f;
    float stunDuration = 0.45f;
    bool isFlying = false;
    bool ignoresWalls = false;
    bool attackOnLastFrame = true;
    MonsterAttackPattern attackPattern = MonsterAttackPattern::Standard;
};

class Player;

class Monster : public Actor {
public:
    MonsterState state = MonsterState::Idle;

    void init(const std::string& atlasKey = "Monster") override;
    Monster(const std::string& type,
        Status status = { kDefaultMaxHp, kDefaultMaxHp, kDefaultPower, kDefaultDex },
        const std::string& atlasKey = "Monster",
        MonsterBehaviorConfig behavior = {})
        : Actor(status), m_type(type) {
        setBehavior(behavior);
        init(atlasKey);
    }

    void update(float dt, Player& player);
    void resetForReuse(Status newStatus, MonsterBehaviorConfig behavior = {});
    void resetForReuse(const std::string& type, Status newStatus,
        const std::string& atlasKey = "Monster", MonsterBehaviorConfig behavior = {});

    void beginAttack();
    void beginSpawn(float duration);
    bool isSpawning() const { return m_spawnActivationTimer > 0.f; }
    void render(sf::RenderWindow& window) override;
    bool consumeAttackAction();
    bool consumeChargeImpact();
    bool readyForPoolRelease() const;
    bool isFlying() const { return m_isFlying; }
    bool ignoresWalls() const { return m_behavior.ignoresWalls; }
    bool isAttackActionReady() const { return m_attackActionReady; }
    bool isChargeImpactActive() const;
    bool requiresBodyContactAttack() const { return m_type == "SkelDog"; }
    float getChargeStunDuration() const { return m_behavior.stunDuration; }
    float getFacingDirection() const { return fsm.facingDirection; }
    MonsterAttackPattern getAttackPattern() const { return m_behavior.attackPattern; }
    const std::string& getType() const { return m_type; }
    bool isTargetInAttackRange(const sf::Vector2f& targetPosition) const;

private:
    std::string m_type;
    int dropGold = 0;
    MonsterFSMData fsm;
    MonsterBehaviorConfig m_behavior;
    float m_attackCooldown = 0.f;
    bool m_attackActionConsumed = false;
    bool m_attackActionReady = false;
    bool m_chargeImpactConsumed = false;
    bool m_isFlying = false;
    bool m_preferGenericDeathAnimation = false;
    float m_chargeDirection = 1.f;
    float m_spawnActivationTimer = 0.f;
    float m_spawnEffectDuration = 0.f;

    void setBehavior(MonsterBehaviorConfig behavior);
    bool canStartAttack() const;
    void startAttackCooldown();
    void updateAttackCooldown(float dt);
    void changeState(MonsterState newState);
    void handleFSM(float dt, const Player& player);
};
