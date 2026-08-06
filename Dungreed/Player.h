#pragma once
#include "Actor.h"
#include "Controller.h"
#include <cstdint> // uint64_t 사용, 모든 상황에서 고정된 비트수를사용하기 위함
enum PlayerState : uint64_t {
    Idle   = 1 << 0, // 1
    Run    = 1 << 1, // 2
    Jump   = 1 << 2, // 4
    Dash   = 1 << 3, // 8
    Attack = 1 << 4, // 16
    Damage = 1 << 5, // 32
    Dead   = 1 << 6  // 64

};

class Player : public Actor {
public:
    uint32_t state = PlayerState::Idle;
    void init(const std::string& atlasKey = "Player") override;
    Player(Status _status = { MAXHP, MAXHP, POWER, DEX }) :Actor(_status) { init("Player"); };
    void update(float dt, const sf::RenderWindow& window);
private:
    Controller controller;
};
