#pragma once

#include "Actor.h"
#include <SFML/System/Vector2.hpp>
#include <string>

enum class MonsterState {
    Idle,
    Patrol,
    Chase,
    Attack,
    Dead
};

struct MonsterFSMData {
    float m_stateTimer = 0.f;
    float m_patrolDir = 1.f;
    float DETECT_RANGE = 100.f;
    float ATTACK_RANGE = 50.f;
};

/// 몬스터 생성 시 전달하는 탐지·공격 거리 설정입니다.
struct MonsterBehaviorConfig {
    float detectRange = 100.f;
    float attackRange = 50.f;
    float moveSpeed = 300.f;
};

class Player;

class Monster : public Actor {
public:
    MonsterState state = MonsterState::Idle;

    void init(const std::string& atlasKey = "Monster") override;
    Monster(const std::string& type,
        Status status = { MAXHP, MAXHP, POWER, DEX },
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
    bool consumeAttackCooldown(float dt);
    bool readyForPoolRelease() const;
    bool isFlying() const { return m_isFlying; }
    bool isTargetInAttackRange(const sf::Vector2f& targetPosition) const;

private:
    std::string m_type;
    int dropGold = 0;
    MonsterFSMData fsm;
    float m_attackCooldown = 0.f;
    bool m_isFlying = false;

    void setBehavior(MonsterBehaviorConfig behavior);
    void changeState(MonsterState newState);
    void handleFSM(float dt, const Player& player);
};