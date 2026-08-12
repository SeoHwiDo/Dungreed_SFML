#pragma once

#include "Actor.h"
#include "Controller.h"

#include <string>
#include <vector>

class TileMap;

enum class PlayerState { Idle, Run, Jump, Dash, Dead };

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

    Player(Status defaultStatus, std::string atlasKey);
    void init(const std::string &atlasKey) override;

    void update(float dt, const sf::RenderWindow &window, const TileMap &tileMap);
    void render(sf::RenderWindow &window) override;
    using Actor::takeDamage;
    /// 대시 중에는 체력·넉백·피격 효과를 적용하지 않습니다.
    void takeDamage(float damage, const sf::Vector2f &attackerPosition, float knockbackMultiplier) override;

    void setDashConfig(DashConfig config);
    const DashConfig &getDashConfig() const { return m_dashConfig; }
    int getDashCharges() const { return m_dashCharges; }
    int getDashMaxCharges() const { return m_dashConfig.maxCharges; }
    float getDashRechargeProgress() const;
    bool isDashing() const { return m_isDashing; }
    void cancelDash();
    void applyStun(float duration);
    bool isStunned() const { return m_stunTimer > 0.f; }
    bool ignoresOneWayPlatforms() const { return m_isDashing || m_ignoreOneWayPlatforms || m_dropThroughTimer > 0.f; }
    void restoreDashCharges(int amount);
    /// 마을 복귀 시 던전에서 누적된 생존·능력치 상태를 기본값으로 되돌립니다.
    void restoreForVillage();
    /// 데이터 파일의 default/easy 프리셋을 기록합니다.
    void configureStatPresets(Status defaultStatus, Status easyStatus);
    /// Debug F7 입력으로 easy 프리셋을 즉시 적용합니다.
    void activateEasyMode();
    bool isEasyMode() const { return m_isEasyMode; }

  private:
    struct DashAfterimage {
        explicit DashAfterimage(const sf::Sprite &source) : sprite(source) {}
        sf::Sprite sprite;
        float remainingTime = 0.f;
        bool active = false;
    };

    Controller controller;
    DashConfig m_dashConfig;
    int m_dashCharges = 3;
    float m_dashRechargeTimer = 0.f;
    bool m_isDashing = false;
    bool m_ignoreOneWayPlatforms = false;
    float m_dropThroughTimer = 0.f;
    float m_dashElapsed = 0.f;
    sf::Vector2f m_dashDelta;
    float m_afterimageTimer = 0.f;
    float m_stunTimer = 0.f;
    float m_facingDirection = 1.f;
    Status m_defaultStatus;
    Status m_easyStatus;
    bool m_isEasyMode = false;
    std::vector<DashAfterimage> m_dashAfterimages;

    void changeState(PlayerState newState);
    void handleState(float dt, const InputData &input, const TileMap &tileMap);
    void updateFacingDirection(const sf::Vector2f &aimWorldPosition);
    bool tryStartDash(const sf::Vector2f &cursorPosition);
    void tryAttack();
    void updateDash(float dt, const TileMap &tileMap);
    void updateDashRecharge(float dt);
    void updateAfterimages(float dt);
    void spawnDashAfterimage();
    void prewarmDashAfterimages();
};
