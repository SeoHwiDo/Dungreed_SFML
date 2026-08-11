#pragma once

#include <SFML/Graphics.hpp>

#include <memory>
#include <string>

class Camera;
class GameplayContext;
class Player;
class SkelBoss;

/// 실제 던전의 층별 초기화와 플레이를 관리합니다.
class DungeonScene {
public:
    DungeonScene(sf::RenderWindow& window, GameplayContext& gameplay);
    ~DungeonScene();

    bool enter(unsigned int floorNumber);
    void update(float dt);
    void render();
    bool spawnDebugRoom(const std::string& floorId, const std::string& roomId);
    void toggleCombatBounds();
    bool isReady() const;
    unsigned int getFloorNumber() const { return m_floorNumber; }
    bool consumeBossDefeat();

private:
    bool placePlayerAtCurrentRoom();

    sf::RenderWindow& m_window;
    GameplayContext& m_gameplay;
    std::unique_ptr<SkelBoss> m_activeBoss;
    unsigned int m_floorNumber = 0;
    bool m_areMonstersActivated = false;
    bool m_showCombatBounds = false;
    bool m_bossDefeated = false;
};
