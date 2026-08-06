#include "Monster.h"
#include "ResourceManager.h"
#include <iostream>
#include <cmath>   // std::sqrt
#include <cstdlib> // std::rand

void Monster::init(const std::string& atlasKey) {
    Actor::init(atlasKey);

    auto& resMgr = ResourceManager::getInstance();
    std::vector<std::string> allAnims = resMgr.getAnimationNames(atlasKey);

    for (const auto& animName : allAnims) {
        // 전체 애니메이션 중, 현재 몬스터의 타입 이름이 포함된 경우만 필터링
        if (animName.find(m_type) != std::string::npos) {
            const auto* frames = resMgr.getAnimationFrames(atlasKey, animName);
            if (frames) {
                // 단발성 애니메이션 추론 (공격, 사망)
                bool isLoop = true;
                if (animName.find("Attack") != std::string::npos ||
                    animName.find("Dead") != std::string::npos) {
                    isLoop = false;
                }

                AnimationClip clip(frames, 0.1f, isLoop);
                animator.addAnimation(animName, clip);
            }
        }
    }
    animator.play(m_type + "_Idle");
}

void Monster::changeState(MonsterState newState) {
    if (state == MonsterState::Dead) return; // 이미 사망한 경우 상태 변경 무시

    state = newState;
    fsm.m_stateTimer = 0.f; // 상태 타이머 초기화

    // 상태에 따른 초기 설정 및 애니메이션 재생
    switch (state) {
    case MonsterState::Idle:
        setHorizontalInput(0.f); // 대기 시 정지
        animator.play(m_type + "_Idle");
        break;

    case MonsterState::Patrol:
        // 무작위로 좌우 방향 결정
        //단순한 좌우 랜덤용이므로 고전 rand 사용
        fsm.m_patrolDir = (std::rand() % 2 == 0) ? 1.f : -1.f;
        animator.play(m_type + "_Run");
        break;

    case MonsterState::Chase:
        animator.play(m_type + "_Run");
        break;

    case MonsterState::Attack:
        setHorizontalInput(0.f); // 공격 시 일단 정지
        animator.play(m_type + "_Attack");
        break;

    case MonsterState::Dead:
        setHorizontalInput(0.f);
        animator.play(m_type + "_Dead");
        break;
    }
}

void Monster::handleFSM(float dt) {
    // 1. 사망 체크
    if (status.tmpHp <= 0 && state != MonsterState::Dead) {
        changeState(MonsterState::Dead);
        return;
    }
    if (state == MonsterState::Dead) return; // 사망 시 AI 중지

    fsm.m_stateTimer += dt;
    sf::Vector2f centerPos = getCenterPosition();

    // 2. 타겟과의 거리 및 방향 계산
    float distToTarget = 9999.f;
    float dirToTargetX = 0.f;

    if (fsm.m_hasTarget) {
        float dx = fsm.m_targetPos.x - centerPos.x;
        float dy = fsm.m_targetPos.y - centerPos.y;
        //삼각함수로 거리계산
        distToTarget = std::sqrt(dx * dx + dy * dy);
        dirToTargetX = (dx > 0.f) ? 1.f : -1.f;
    }

    // 3. 상태별 로직 처리
    switch (state) {
    case MonsterState::Idle:
        // 시야 범위 내에 타겟이 들어오면 추적 시작
        if (fsm.m_hasTarget && distToTarget <= fsm.DETECT_RANGE) {
            changeState(MonsterState::Chase);
        }
        // 2초 정도 대기 후 순찰 시작
        else if (fsm.m_stateTimer > 2.0f) {
            changeState(MonsterState::Patrol);
        }
        break;

    case MonsterState::Patrol:
        // 걷는 속도는 절반만 사용 (0.5f)
        setHorizontalInput(fsm.m_patrolDir * 0.5f);

        if (sprite) {
            sprite->setScale({ fsm.m_patrolDir, 1.f });
        }

        if (fsm.m_hasTarget && distToTarget <= fsm.DETECT_RANGE) {
            changeState(MonsterState::Chase);
        }
        // 3초간 순찰 후 다시 대기
        else if (fsm.m_stateTimer > 3.0f) {
            changeState(MonsterState::Idle);
        }
        break;

    case MonsterState::Chase:
        // 타겟을 상실했거나 추적 범위를 크게 벗어나면 포기
        if (!fsm.m_hasTarget || distToTarget > fsm.DETECT_RANGE * 1.5f) {
            changeState(MonsterState::Idle);
        }
        // 공격 사거리 내에 들어왔을 경우
        else if (distToTarget <= fsm.ATTACK_RANGE) {
            changeState(MonsterState::Attack);
        }
        // 계속 추적 (전속력)
        else {
            setHorizontalInput(dirToTargetX);
            if (sprite) {
                sprite->setScale({ dirToTargetX, 1.f });
            }
        }
        break;

    case MonsterState::Attack:
        // 공격 애니메이션(단발성)이 완전히 끝났는지 확인
        if (animator.isFinished()) {
            // 공격 후 잠시 대기 상태로 전환 (연속 공격 방지 쿨타임 역할)
            changeState(MonsterState::Idle);
        }
        break;
    }
}

void Monster::update(float dt) {
    // 1. 상태 머신 로직 업데이트
    handleFSM(dt);

    // 2. 부모 클래스(Actor)의 물리 연산 및 애니메이션 갱신 처리
    Actor::update(dt);
}