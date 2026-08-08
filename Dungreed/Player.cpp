#include "Player.h"
#include"ResourceManager.h"
#include"Equip.h"
void Player::init(const std::string& atlasKey) {
    Actor::init(atlasKey);
    if (!equipment) {
        auto defaultWeapon = std::make_shared<Equip>("ShortSword", EquipStat{ 10.f, 2.5f, 40.f });
        defaultWeapon->init("Equip", "ShortSword_Idle-00");
        setEquipment(defaultWeapon);
    }
    //애니메이션 가져오기
    auto& resMgr = ResourceManager::getInstance();
    std::vector<std::string> allAnims = resMgr.getAnimationNames(atlasKey);

    for (const auto& animName : allAnims) {
        const auto* frames = resMgr.getAnimationFrames(atlasKey, animName);
        if (frames) {
            // 공격과 사망 애니메이션은 마지막 프레임에서 멈춥니다.
            bool isLoop = true;
            if (animName.find("Attack") != std::string::npos ||
                animName.find("Dead") != std::string::npos ||
                animName.find("Die") != std::string::npos) {
                isLoop = false;
            }

            float frameDuration = 0.15f; // 기본 속도

            if (animName.find("Idle") != std::string::npos) {
                frameDuration = 0.2f;  // 대기는 천천히 (초당 5프레임)
            } else if (animName.find("Run") != std::string::npos) {
                frameDuration = 0.05f; // 걷기/뛰기는 조금 빠르게
            } else if (animName.find("Attack") != std::string::npos) {
                frameDuration = 0.08f; // 공격은 아주 역동적이고 빠르게
            } else if (animName.find("Jump") != std::string::npos) {
                frameDuration = 0.15f;
            }
            AnimationClip clip(frames, frameDuration, isLoop);
            animator.addAnimation(animName, clip);
        }
    }
    
    // 4. 초기 상태 애니메이션 실행
    animator.play("Player_Idle");
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
    case PlayerState::Dead:
        // 사망 직전의 이동, 점프, 넉백을 모두 제거합니다.
        setHorizontalInput(0.f);
        movement.velocity.y = 0.f;
        m_knockbackVelocity = { 0.f, 0.f };
        m_knockbackTimer = 0.f;
        playAnimation("Player_Die");
        break;
    }
}

void Player::handleState(float dt, const InputData& input) {
    if (dead()) {
        changeState(PlayerState::Dead);
        return;
    }

    setHorizontalInput(input.moveDirX);

    // 마우스 위치로부터 추출된 단위 벡터의 x 방향을 기준으로 스프라이트 좌우 반전
    if (sprite) {
        if (input.aimDir.x < 0.f) {
            sprite->setScale({ -1.f, 1.f });
        }
        else {
            sprite->setScale({ 1.f, 1.f });
        }
    }
    if (input.isJumping && movement.isGrounded) {
        jump();
    }

    if (!movement.isGrounded) {
        changeState(PlayerState::Jump);
    } else if (input.moveDirX != 0.f) {
        changeState(PlayerState::Run);
    }
    else {
        changeState(PlayerState::Idle);
    }

    if (input.isAttacking && equipment) {
        equipment->attack();
    }
    if (equipment) {
        equipment->update(dt, getBodyCenterPosition(), input.aimRadian);
    }
    if (input.isDashing) {
        // 대시 이펙트 및 특수 물리 로직 처리
        // 예: EffectManager::spawnTrail(sprite->getPosition()); (잔상 이펙트)
        // 예: movement.velocity.x = dashSpeed * input.moveDirX; (순간 가속)
    }
}

void Player::update(float dt, const sf::RenderWindow& window)  {
    // 사망한 뒤에는 입력을 무시하고, 중력/피격 피드백/애니메이션만 갱신합니다.
    if (dead()) {
        changeState(PlayerState::Dead);
        updatePhysics(dt);
        updateHitFeedback(dt);
        updateAnimation(dt);
        return;
    }

    // 입력 -> 상태 전환 -> 물리/피격 처리 -> 애니메이션 순서를 유지합니다.
    const InputData input = controller.getInput(window, getBodyCenterPosition());
    handleState(dt, input);
    updatePhysics(dt);
    updateHitFeedback(dt);
    updateAnimation(dt);
}
