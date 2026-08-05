#pragma once
#include "Actor.h"
enum class PlayerState {
    Idle,
    RUN,
    JUMP,
    FALL,
    ATTACK,
    DEAD
};

class Player : public Actor {
public:
    PlayerState state = PlayerState::Idle;
    // std::list<Item> Inventory;       
    // std::shared_ptr<Equip> equipment; 

    Player() = default;

    inline void update(float dt) override {
        // 키보드 입력에 따른 상태 변경 로직 (컨트롤러 역할)
        Actor::update(dt);
    }
};
