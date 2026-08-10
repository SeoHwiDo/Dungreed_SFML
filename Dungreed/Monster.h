#pragma once

#include "Actor.h"

#include <SFML/System/Vector2.hpp>

#include <string>
#include <unordered_map>

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

/// Attack 상태 안에서 준비 애니메이션·실제 판정·후딜을 구분합니다.
enum class MonsterAttackPhase {
    Ready,
    Active,
    Recovery
};

struct MonsterFSMData {
    float stateTimer = 0.f;
    float patrolDirection = 1.f;
    float patrolVerticalDirection = 1.f;
    float facingDirection = 1.f;
    float detectRange = 100.f;
    float attackRange = 50.f;
};

/// 몬스터 종류별 JSON에서 읽는 애니메이션 재생 규칙입니다.
struct MonsterAnimationConfig {
    bool isLoop = true;
    float frameDuration = 0.2f;
};

struct MonsterBehaviorConfig {
    float detectRange = 100.f;
    float attackRange = 50.f;
    float moveSpeed = 300.f;
    float gravity = 980.f;
    float idleDuration = 2.f;
    float patrolDuration = 1.f;
    float patrolSpeedMultiplier = 0.2f;
    float chaseExitRangeMultiplier = 1.5f;
    float attackWindup = 0.35f;
    float attackReadyDuration = 0.4f;
    float attackRecoveryDuration = 0.4f;
    float attackActiveTimeout = 1.5f;
    float chargeDuration = 0.55f;
    float chargeSpeedMultiplier = 3.f;
    float stunDuration = 0.45f;
    bool isFlying = false;
    bool ignoresWalls = false;
    bool lockAttackFacing = false;
    bool playReleaseAnimation = false;
    bool waitForAttackCooldownAfterRelease = false;
    MonsterAttackPattern attackPattern = MonsterAttackPattern::Standard;
    /// 상태 키(Idle, Attack, Release 등)별 애니메이션 재생 규칙입니다.
    std::unordered_map<std::string, MonsterAnimationConfig> animations;
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
    /// 소환 중 행동을 멈추고, revealDelay가 끝난 뒤에만 스프라이트를 표시합니다.
    void beginSpawn(float duration, float revealDelay = 0.f);
    bool isSpawning() const { return m_spawnActivationTimer > 0.f; }
    void render(sf::RenderWindow& window) override;
    bool consumeAttackAction();
    bool consumeChargeImpact();
    /// 현재 설정된 속도와 지속 시간으로 이동 가능한 최대 돌진 거리를 반환합니다.
    float getChargeDistance() const;
    bool readyForPoolRelease() const;
    bool isFlying() const { return m_isFlying; }
    bool ignoresWalls() const { return m_behavior.ignoresWalls; }
    bool isAttackActionReady() const { return m_attackActionReady; }
    /// 실제 공격 판정이 가능한 프레임인지 반환합니다. 디버그 범위 표시에 사용합니다.
    bool isAttackDamageWindowActive() const {
        return state == MonsterState::Attack &&
            m_attackPhase == MonsterAttackPhase::Active && m_attackActionReady;
    }
    bool isChargeImpactActive() const;
    /// 바라보는 방향의 몸체 전면 절반을 근접 공격의 실제 충돌 영역으로 반환합니다.
    sf::FloatRect getFrontAttackBounds() const;
    /// 대상이 몬스터 전면 충돌 영역과 실제로 겹치는지 검사합니다.
    bool isTargetInFrontContact(const sf::FloatRect& targetBounds) const;
    float getChargeStunDuration() const { return m_behavior.stunDuration; }
    /// 플레이어를 추적하기 시작하는 탐지 반지름을 반환합니다.
    float getDetectRange() const { return fsm.detectRange; }
    /// 현재 공격 판정에서 목표와의 거리를 검사할 때 사용하는 반지름을 반환합니다.
    float getAttackRange() const { return fsm.attackRange; }
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
    MonsterAttackPhase m_attackPhase = MonsterAttackPhase::Active;
    float m_attackFacingDirection = 1.f;
    bool m_attackWasInterruptedByHit = false;
    bool m_attackReleasePlayed = false;
    bool m_chargeImpactConsumed = false;
    bool m_isFlying = false;
    bool m_preferGenericDeathAnimation = false;
    float m_chargeDirection = 1.f;
    float m_spawnActivationTimer = 0.f;
    float m_spawnEffectDuration = 0.f;
    float m_spawnRevealTimer = 0.f;

    void setBehavior(MonsterBehaviorConfig behavior);
    bool canStartAttack() const;
    void startAttackCooldown();
    void updateAttackCooldown(float dt);
    void playAttackAnimation();
    bool playAttackReleaseAnimation();
    void changeState(MonsterState newState);
    void handleFSM(float dt, const Player& player);

};
