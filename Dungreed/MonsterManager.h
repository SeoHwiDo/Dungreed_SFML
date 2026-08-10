#pragma once

#include <random>
#include <string>
#include <vector>

#include "ObjectPoolingManager.h"

class GameDataManager;
struct MonsterData;
class Player;
class Room;
class TileMap;

class MonsterManager {
public:
    void requestRoomMonsters(Room& room, const TileMap& tileMap,
        const GameDataManager& gameData, ObjectPoolingManager& objectPool,
        const sf::Vector2f& playerPosition);
    void update(float dt, Player& player, ObjectPoolingManager& objectPool,
        const TileMap& tileMap);
    void clearActiveRoom(ObjectPoolingManager& objectPool);

private:
    Room* m_activeRoom = nullptr;
    const GameDataManager* m_gameData = nullptr;
    const TileMap* m_activeTileMap = nullptr;
    std::vector<Monster*> m_activeRoomMonsters;
    int m_totalPhaseCount = 0;
    int m_currentPhase = 0;
    float m_phaseDelayTimer = 0.f;
    std::mt19937 m_randomEngine{ std::random_device{}() };

    void prepareRoomEncounter(Room& room, const GameDataManager& gameData);
    bool spawnMonster(const MonsterData& monsterData,
        std::vector<sf::Vector2f>& spawnCandidates,
        const sf::Vector2f& positionOffset, float activationDelay,
        const sf::Vector2f& playerPosition,
        ObjectPoolingManager& objectPool);
    void spawnNextPhase(const sf::Vector2f& playerPosition,
        ObjectPoolingManager& objectPool);
    void releaseActiveRoomMonsters(ObjectPoolingManager& objectPool);
};
