#pragma once

#include <SFML/Graphics.hpp>

#include <optional>
#include <string>
#include <vector>

class Player;
class Boss;

class UIManager {
  public:
    static UIManager &getInstance() {
        static UIManager instance;
        return instance;
    }

    UIManager(const UIManager &) = delete;
    UIManager &operator=(const UIManager &) = delete;

    bool init(sf::RenderWindow &window);
    void update(const Player &player, float dt, const sf::RenderWindow &window);
    void render(sf::RenderWindow &window) const;
    void attachBoss(const Boss *boss);
    void detachBoss(const Boss *boss);

  private:
    UIManager() = default;
    ~UIManager() = default;

    struct DashSlot {
        DashSlot(const sf::Texture &uiTexture, const sf::Texture &flashTexture) : base(uiTexture), count(uiTexture), flash(flashTexture) {}

        sf::Sprite base;
        sf::Sprite count;
        sf::Sprite flash;
        sf::Vector2f basePivot;
        float flashTimer = 0.f;
    };

    static constexpr float kHealthBarOffsetX = 22.f;
    static constexpr float kHealthBarOffsetY = 3.f;
    static constexpr float kHealthBarWidth = 49.f;
    static constexpr float kUIScale = 5.f;
    static constexpr float kWaveFrameDuration = 0.08f;
    static constexpr float kDashChargeFlashDuration = 0.18f;
    static constexpr float kBossHudScale = 4.f;

    const sf::Texture *m_texture = nullptr;
    std::optional<sf::Texture> m_lifeBarTexture;
    std::optional<sf::Texture> m_dashCountFlashTexture;
    sf::IntRect m_dashCountFrame;
    std::optional<sf::Sprite> m_lifeBack;
    std::optional<sf::Sprite> m_lifeBar;
    std::optional<sf::Sprite> m_lifeBase;
    std::vector<sf::Sprite> m_lifeWaves;
    std::vector<DashSlot> m_dashSlots;
    std::optional<sf::Sprite> m_dashRightEnd;
    std::optional<sf::Sprite> m_cursor;
    std::optional<sf::Text> m_playerHealthText;
    std::optional<sf::Text> m_bossNameText;
    std::optional<sf::Sprite> m_bossLifeFill;
    std::optional<sf::Sprite> m_bossLifeBase;
    std::optional<sf::Sprite> m_bossPortrait;
    sf::IntRect m_bossLifeBackFrame;
    sf::Vector2f m_healthPosition{16.f, 16.f};
    sf::Vector2f m_dashPosition{16.f, 108.f};
    float m_previousHealthRatio = 1.f;
    float m_waveElapsed = 0.f;
    std::size_t m_waveIndex = 0;
    int m_previousDashCharges = -1;
    bool m_showLifeWave = false;
    bool m_showBossHud = false;
    const Boss *m_activeBoss = nullptr;

    bool setSpriteFrame(sf::Sprite &sprite, const std::string &frameName) const;
    bool createScaledLifeBarTexture();
    bool createDashCountFlashTexture();
    void updateHealth(float currentHp, float maxHp, float dt);
    void updatePlayerHealthText(float currentHp, float maxHp);
    void updateDashCharges(int charges, float rechargeProgress, float dt);
    void updateBossHud(const sf::RenderWindow &window);
};
