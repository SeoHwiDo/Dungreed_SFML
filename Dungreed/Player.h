#pragma once
#include "Actor.h"
#include "Controller.h"
enum class PlayerState {
    Idle,
    Run,
    Jump,
    Dead
};

class Player : public Actor {
public:
    /// 현재 플레이어 상태입니다. 입력 해석과 애니메이션 선택의 기준으로 사용합니다.
    PlayerState state = PlayerState::Idle;
    /// 플레이어 아틀라스에서 상태별 애니메이션을 등록합니다.
    void init(const std::string& atlasKey = "Player") override;
    /// 지정 능력치로 생성하고 기본 플레이어 아틀라스를 즉시 초기화합니다.
    Player(Status _status = { MAXHP, MAXHP, POWER, DEX }) :Actor(_status) { init("Player"); };
    /// 입력 수집 → 상태 처리 → 물리/피격 처리 → 애니메이션 순으로 한 프레임을 갱신합니다.
    void update(float dt, const sf::RenderWindow& window);
private:
    Controller controller;

    /// 상태가 실제로 바뀔 때만 해당 애니메이션을 시작합니다.
    void changeState(PlayerState newState);
    /// 현재 상태와 입력을 해석해 이동·점프·공격을 처리합니다. Dead 상태에서는 조작을 무시합니다.
    void handleState(float dt, const InputData& input);
};
