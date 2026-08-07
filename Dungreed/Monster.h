#pragma once
#include"Actor.h"
#include <string>
#include <SFML/System/Vector2.hpp>
enum class MonsterState {
    Idle,
    Patrol,
    Chase,
    Attack,
    Dead
};
struct MonsterFSMData {
    float m_stateTimer = 0.f;           // 상태 유지 시간 기록
    float m_patrolDir = 1.f;            // 순찰 방향 (1.f: 우측, -1.f: 좌측)
// 시야 및 공격 반경 (필요시 몬스터 타입별로 설정할 수도 있습니다)
    float DETECT_RANGE = 100.f;
    float ATTACK_RANGE = 50.f;
};
class Player;

class Monster : public Actor {
public:
    /// 현재 몬스터 상태입니다. FSM 행동과 애니메이션 선택에 사용합니다.
    MonsterState state = MonsterState::Idle;
    /// 몬스터 아틀라스와 공통 Monster_Die 애니메이션을 등록합니다.
    void init(const std::string& atlasKey = "Monster") override;
    /// 타입·능력치·아틀라스를 설정하고 몬스터를 초기화합니다.
    Monster(const std::string& type, Status _status = { MAXHP, MAXHP, POWER, DEX },const std::string & atlasKey = "Monster") :Actor(_status), m_type(type) { init(atlasKey); }
    
    /// 플레이어를 대상으로 FSM 행동, 공격 판정, 공통 물리/애니메이션을 한 프레임 갱신합니다.
    void update(float dt, Player& player);
private:
    std::string m_type;
    int dropGold = 0;
    MonsterFSMData fsm;
    /// 상태가 바뀔 때만 타이머·이동값·애니메이션을 새 상태에 맞게 초기화합니다.
    void changeState(MonsterState newState);
    /// 플레이어와의 거리로 대기·순찰·추적·공격 행동을 결정합니다.
    void handleFSM(float dt, const Player& player);
};
