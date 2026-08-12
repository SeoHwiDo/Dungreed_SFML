#pragma once

#include <SFML/Graphics.hpp>

#include <optional>

/// 시작 화면의 리소스, 입력, 애니메이션과 렌더링을 관리합니다.
class TitleScene {
  public:
    explicit TitleScene(sf::RenderWindow &window) : m_window(window) {}

    bool enter();
    void update(float dt);
    void render();

  private:
    void placeStartText();

    sf::RenderWindow &m_window;
    const sf::Texture *m_logoTexture = nullptr;
    const sf::Texture *m_skyTexture = nullptr;
    const sf::Texture *m_backCloudTexture = nullptr;
    const sf::Texture *m_midCloud0Texture = nullptr;
    const sf::Texture *m_midCloud1Texture = nullptr;
    const sf::Texture *m_frontCloudTexture = nullptr;
    std::optional<sf::Text> m_startText;
    float m_elapsed = 0.f;
};
