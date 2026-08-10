#pragma once

#include <SFML/Graphics.hpp>

#include <optional>
#include <string>
#include <vector>

class Player;

class UIManager {
public:
    static UIManager& getInstance() {
        static UIManager instance;
        return instance;
    }

    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;

    bool init(sf::RenderWindow& window);
    void update(const Player& player, float dt, const sf::RenderWindow& window);
    void render(sf::RenderWindow& window) const;

private:
    UIManager() = default;
    ~UIManager() = default;

    static constexpr float kHealthBarOffsetX = 22.f;
    static constexpr float kHealthBarOffsetY = 3.f;
    static constexpr float kHealthBarWidth = 49.f;
    static constexpr float kUIScale = 5.f;
    static constexpr float kWaveFrameDuration = 0.08f;

    const sf::Texture* m_texture = nullptr;
    std::optional<sf::Texture> m_lifeBarTexture;
    std::optional<sf::Sprite> m_lifeBack;
    std::optional<sf::Sprite> m_lifeBar;
    std::optional<sf::Sprite> m_lifeBase;
    std::vector<sf::Sprite> m_lifeWaves;
    std::vector<sf::Sprite> m_dashBases;
    std::vector<sf::Sprite> m_dashCounts;
    std::optional<sf::Sprite> m_cursor;
    sf::Vector2f m_healthPosition{ 16.f, 16.f };
    sf::Vector2f m_dashPosition{ 16.f, 108.f };
    float m_previousHealthRatio = 1.f;
    float m_waveElapsed = 0.f;
    std::size_t m_waveIndex = 0;
    bool m_showLifeWave = false;

    bool setSpriteFrame(sf::Sprite& sprite, const std::string& frameName) const;
    bool createScaledLifeBarTexture();
    void updateHealth(float currentHp, float maxHp, float dt);
    void updateDashCharges(int charges);
};
