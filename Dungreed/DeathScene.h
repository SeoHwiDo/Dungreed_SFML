#pragma once

#include <SFML/Graphics.hpp>

#include <optional>

enum class ResultType { Failure, Success };

/// 전투 결과 시 현재 장면을 정지 화면으로 보관하고, 결과 안내와 복귀 입력을 표시합니다.
class DeathScene {
  public:
    explicit DeathScene(sf::RenderWindow &window) : m_window(window) {}

    bool init(const sf::Font &font);
    bool enter(ResultType resultType);
    void leave();

    bool isActive() const { return m_active; }
    bool needsCapture() const { return m_captureRequested; }
    bool handleEvent(const sf::Event &event) const;

    void captureCurrentScreen();
    void render();

  private:
    void placeTexts();

    sf::RenderWindow &m_window;
    sf::Texture m_snapshotTexture;
    std::optional<sf::Sprite> m_snapshotSprite;
    std::optional<sf::Sprite> m_resultSprite;
    std::optional<sf::Text> m_messageText;
    std::optional<sf::Text> m_helpText;
    ResultType m_resultType = ResultType::Failure;
    bool m_active = false;
    bool m_captureRequested = false;
};
