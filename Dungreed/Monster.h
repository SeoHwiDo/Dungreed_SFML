#pragma once
#include"Actor.h"
#include <string>
#include <utility>
enum class MonsterState {
    Idle,
    Patrol,
    Chase,
    Attack,
    Dead
};

class Monster : public Actor {
public:
    Monster(std::string type, Status _status, sf::Sprite monsterSprite);
    MonsterState state = MonsterState::Idle;
   
    void update(float dt) override;
private:
    std::string m_type;
    int dropGold = 0;
};