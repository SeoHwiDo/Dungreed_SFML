#pragma once
#include "Actor.h"
#include "Controller.h"
#include <cstdint> // uint32_t 사용, 모든 상황에서 고정된 비트수를사용하기 위함
enum PlayerState : uint32_t {
    Idle   = 1 << 0, // 1
    Run    = 1 << 1, // 2
    Jump   = 1 << 2, // 4
    Dash   = 1 << 3, // 8
    Attack = 1 << 4, // 16
    Dead   = 1 << 5  // 32
};

class Player : public Actor {
public:
    uint32_t state = PlayerState::Idle;
    // std::list<Item> Inventory;       
    // std::shared_ptr<Equip> equipment; 


    void update(float dt, const sf::RenderWindow& window);
private:
    Controller controller;
};
