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
    PlayerState state = PlayerState::Idle;
    void init(const std::string& atlasKey = "Player") override;
    Player(Status _status = { MAXHP, MAXHP, POWER, DEX }) :Actor(_status) { init("Player"); };
    void update(float dt, const sf::RenderWindow& window);
private:
    Controller controller;

    void changeState(PlayerState newState);
    void handleState(float dt, const InputData& input);
};
