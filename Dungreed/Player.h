#pragma once

#include "Actor.h"
#include "Controller.h"

#include <vector>

class TileMap;

enum class PlayerState {
    Idle,
    Run,
    Jump,
    Dash,
    Dead
};

struct DashConfig {
    float maxDistanceMultiplier = 8.f;
    int maxCharges = 3;
    float chargeRecoveryTime = 3.f;
    float duration = 0.12f;
    float afterimageInterval = 0.03f;
    float afterimageLifetime = 0.20f;
};

class Player : public Actor {
public:
    PlayerState state = PlayerState::Idle;

    void init(const std::string& atlasKey = "Player") override;
    Player(Status status = { kDefaultMaxHp, kDefaultMaxHp, kDefaultPower, kDefaultDex }) : Actor(status) { init("Player"); }

    void update(float dt, const sf::RenderWindow& window,
        const TileMap& tileMap);
    void render(sf::RenderWindow& window) override;

    void setDashConfig(DashConfig config);
    const DashConfig& getDashConfig() const { return m_dashConfig; }
    int getDashCharges() const { return m_dashCharges; }
    int getDashMaxCharges() const { return m_dashConfig.maxCharges; }
    float getDashRechargeProgress() const;
    bool isDashing() const { return m_isDashing; }
    void cancelDash();
    void applyStun(float duration);
    bool isStunned() const { return m_stunTimer > 0.f; }
    bool ignoresOneWayPlatforms() const { return m_isDashing || m_ignoreOneWayPlatforms; }
    void restoreDashCharges(int amount);

private:
    struct DashAfterimage {
        sf::Sprite sprite;
        float remainingTime = 0.f;
    };

    Controller controller;
    DashConfig m_dashConfig;
    int m_dashCharges = 3;
    float m_dashRechargeTimer = 0.f;
    bool m_isDashing = false;
    bool m_ignoreOneWayPlatforms = false;
    float m_dashElapsed = 0.f;
    sf::Vector2f m_dashDelta;
    float m_afterimageTimer = 0.f;
    float m_stunTimer = 0.f;
    std::vector<DashAfterimage> m_dashAfterimages;

    void changeState(PlayerState newState);
    void handleState(float dt, const InputData& input);
    bool tryStartDash(const sf::Vector2f& cursorPosition);
    void updateDash(float dt, const TileMap& tileMap);
    void updateDashRecharge(float dt);
    void updateAfterimages(float dt);
    void spawnDashAfterimage();
};