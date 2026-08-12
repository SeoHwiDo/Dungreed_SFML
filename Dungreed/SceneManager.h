#pragma once

#include <SFML/Graphics.hpp>

#include "DeathScene.h"
#include "DungeonScene.h"
#include "SceneTransition.h"
#include "TitleScene.h"
#include "VillageScene.h"

#include <optional>

class GameplayContext;
struct DebugCommand;

/// 활성 씬 선택과 페이드 전환만 담당하며, 게임플레이 상태는 GameplayContext에 위임합니다.
class SceneManager {
  public:
    SceneManager(sf::RenderWindow &window, GameplayContext &gameplay);

    bool init();
    void handleEvent(const sf::Event &event);
    void update(float dt);
    void render();
    bool changeToVillage();
    bool changeToDungeon(unsigned int floorNumber);

    GameScene getActiveScene() const { return m_activeScene; }

  private:
    void handleDebugCommand(const DebugCommand &command);
    void renderActiveScene();
    void activateEasyMode();
    void renderEasyModeIndicator();

    sf::RenderWindow &m_window;
    GameplayContext &m_gameplay;
    TitleScene m_titleScene;
    VillageScene m_villageScene;
    DungeonScene m_dungeonScene;
    DeathScene m_deathScene;
    SceneTransition m_transition;
    GameScene m_activeScene = GameScene::Title;
    std::optional<sf::Text> m_easyModeText;
};
