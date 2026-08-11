#pragma once

#include <SFML/Graphics.hpp>

#include <memory>
#include <optional>

class GameplayContext;
class TileMap;

/// 조작법을 익히는 시작마을을 독립적으로 관리합니다.
class VillageScene {
public:
    VillageScene(sf::RenderWindow& window, GameplayContext& gameplay);
    ~VillageScene();

    bool enter();
    void update(float dt);
    void render();
    bool isReady() const;

private:
    sf::RenderWindow& m_window;
    GameplayContext& m_gameplay;
    TileMap* m_tileMap = nullptr;
    std::optional<sf::Text> m_titleText;
    std::optional<sf::Text> m_helpText;
};
