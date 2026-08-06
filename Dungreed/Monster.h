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

class Monster : public Actor {
public:
    MonsterState state = MonsterState::Idle;
    void init(const std::string& atlasKey = "Monster") override;
    Monster(const std::string& type, Status _status = { MAXHP, MAXHP, POWER, DEX },const std::string & atlasKey = "Monster") :Actor(_status), m_type(type) { init(atlasKey); }
    

  

    void update(float dt) override;
private:
    std::string m_type;
    int dropGold = 0;
};