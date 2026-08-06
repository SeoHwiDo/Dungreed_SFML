#include "Player.h"
#include"ResourceManager.h"
#include"Equip.h"
void Player::init(const std::string& atlasKey) {
    Actor::init(atlasKey);
    if (!equipment) {
        auto defaultWeapon = std::make_shared<Equip>("ShortSword", EquipStat{ 10.f, 2.5f, 40.f });
        defaultWeapon->init("Equip", "ShortSword");
        setEquipment(defaultWeapon);
    }
    //애니메이션 가져오기
    auto& resMgr = ResourceManager::getInstance();
    std::vector<std::string> allAnims = resMgr.getAnimationNames(atlasKey);

    for (const auto& animName : allAnims) {
        const auto* frames = resMgr.getAnimationFrames(atlasKey, animName);
        if (frames) {
            // 이름에 "Attack"이나 "Dead"가 들어가면 반복(Loop) 재생을 끕니다.
            bool isLoop = true;
            if (animName.find("Attack") != std::string::npos ||
                animName.find("Dead") != std::string::npos) {
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

void Player::update(float dt, const sf::RenderWindow& window)  {
    // Actor::update에서 updatePhysics(dt)가 실행됨
    Actor::update(dt);

    state = 0;
    if (status.tmpHp <= 0) {
        state |= PlayerState::Dead;
        return;
    }

    sf::Vector2f centerPos = getCenterPosition();
    InputData input = controller.getInput(window, centerPos);

    // 입력받은 방향 전달
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
    // =========================================================
        // 2. 모든 상황에서 가능한 액션 (조건 없이 플래그 누적)
        // =========================================================
    if (input.isAttacking) {
        state |= PlayerState::Attack;
    }

    if (input.isDashing) {
        state |= PlayerState::Dash;
    }

    if (input.isJumping&&movement.isGrounded) {
        // [추론한 내용]: 점프가 모든 상황에서 가능하다는 조건에 따라, 
        // movement.isGrounded 검사를 제거하고 무조건 jump()를 호출하도록 구현했습니다.
        // (만약 무한 점프를 막아야 한다면 다중 점프 횟수 제한 변수가 별도로 필요합니다.)
        jump();
        state |= PlayerState::Jump;
    }

    if (movement.isGrounded) {
        if (input.moveDirX != 0.f) {
            state |= PlayerState::Run;
        }
        else {
            state |= PlayerState::Idle;
        }
    }
    else {
        // 바닥이 아니면 무조건 점프(공중) 상태 포함
        state |= PlayerState::Jump;
    }

    if (state & PlayerState::Jump) {
        playAnimation("Player_Jump");
    } else if (state & PlayerState::Run) {
        playAnimation("Player_Run");
    } else {
        playAnimation("Player_Idle");
    }
    if (state & PlayerState::Attack) {
        if (equipment) {
            equipment->attack();
        }
    }
    if (equipment) {
        equipment->update(dt, getCenterPosition(), input.aimRadian);
    }
    if (state & PlayerState::Dash) {
        // 대시 이펙트 및 특수 물리 로직 처리
        // 예: EffectManager::spawnTrail(sprite->getPosition()); (잔상 이펙트)
        // 예: movement.velocity.x = dashSpeed * input.moveDirX; (순간 가속)
    }

}
