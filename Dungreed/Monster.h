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
    sf::Vector2f m_targetPos{ 0.f, 0.f }; // 타겟(플레이어)의 위치
    bool m_hasTarget = false;     
    // 시야 및 공격 반경 (필요시 몬스터 타입별로 설정할 수도 있습니다)
    const float DETECT_RANGE = 400.f;
    const float ATTACK_RANGE = 80.f;// 타겟 유무 플래그
};
class Monster : public Actor {
public:
    MonsterState state = MonsterState::Idle;
    void init(const std::string& atlasKey = "Monster") override;
    Monster(const std::string& type, Status _status = { MAXHP, MAXHP, POWER, DEX },const std::string & atlasKey = "Monster") :Actor(_status), m_type(type) { init(atlasKey); }
    
    inline void setTargetPos(const sf::Vector2f& pos) { fsm.m_targetPos = pos; fsm.m_hasTarget = true; }
    inline void clearTarget() { fsm.m_hasTarget = false; }

  

    void update(float dt) override;
private:
    std::string m_type;
    int dropGold = 0;
    MonsterFSMData fsm;
    void changeState(MonsterState newState);
    void handleFSM(float dt);
};