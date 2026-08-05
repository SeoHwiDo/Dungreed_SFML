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
    Monster(std::string type) :m_type(type) {}
    MonsterState state = MonsterState::Idle;
   

    Monster() = default;

    void update(float dt) override {
        // FSM에 따른 행동 결정 로직
        Actor::update(dt);
    }
private:
    std::string m_type;
    int dropGold = 0;
};