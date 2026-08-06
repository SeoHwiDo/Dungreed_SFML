#include "Player.h"
void Player::update(float dt, const sf::RenderWindow& window)  {
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
         animator.play("Player_Jump");
    }
    else if (state & PlayerState::Run) {
         animator.play("Player_Run");
    }
    else if (state & PlayerState::Idle) {
         animator.play("Player_Idle");
    }
    if (state & PlayerState::Attack) {
        // 무기 공격 명령 전달
        // 예: if (equippedWeapon) equippedWeapon->playAttackAnimation(input.aimDir);
    }

    if (state & PlayerState::Dash) {
        // 대시 이펙트 및 특수 물리 로직 처리
        // 예: EffectManager::spawnTrail(sprite->getPosition()); (잔상 이펙트)
        // 예: movement.velocity.x = dashSpeed * input.moveDirX; (순간 가속)
    }
    // Actor::update에서 updatePhysics(dt)가 실행됨
    Actor::update(dt);
}
