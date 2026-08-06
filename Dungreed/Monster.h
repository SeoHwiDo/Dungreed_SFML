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
    float DETECT_RANGE = 200.f;
    float ATTACK_RANGE = 50.f;
};
class Player;

class Monster : public Actor {
public:
    MonsterState state = MonsterState::Idle;
    void init(const std::string& atlasKey = "Monster") override;
    Monster(const std::string& type, Status _status = { MAXHP, MAXHP, POWER, DEX },const std::string & atlasKey = "Monster") :Actor(_status), m_type(type) { init(atlasKey); }
    
    void update(float dt, const Player& player);
private:
    std::string m_type;
    int dropGold = 0;
    MonsterFSMData fsm;
    void changeState(MonsterState newState);
    void handleFSM(float dt, const Player& player);
};