#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "ObjectPoolingManager.h"

class GameDataManager;
struct MonsterData;
class Player;
class Room;
class TileMap;
class EffectManager;

class MonsterManager {
public:
    void requestRoomMonsters(Room& room, const TileMap& tileMap,
        const GameDataManager& gameData, ObjectPoolingManager& objectPool,
        const sf::Vector2f& playerPosition, EffectManager& effectManager);
    void update(float dt, Player& player, ObjectPoolingManager& objectPool,
        const TileMap& tileMap, EffectManager& effectManager);
    void clearActiveRoom(ObjectPoolingManager& objectPool);

private:
    Room* m_activeRoom = nullptr;
    const GameDataManager* m_gameData = nullptr;
    const TileMap* m_activeTileMap = nullptr;
    std::vector<Monster*> m_activeRoomMonsters;
    /// 겹쳐 진행되는 페이즈에서 각 몬스터가 어느 페이즈 소속인지 보관합니다.
    std::unordered_map<Monster*, int> m_monsterPhaseIndices;
    int m_totalPhaseCount = 0;
    int m_currentPhase = 0;
    bool m_isWaitingForMidpoint = false;
    void prepareRoomEncounter(Room& room, const GameDataManager& gameData);
    bool spawnMonster(const MonsterData& monsterData,
        std::vector<sf::Vector2f>& spawnCandidates,
        const sf::Vector2f& positionOffset, float activationDelay,
        const sf::Vector2f& playerPosition,
        int phaseIndex, ObjectPoolingManager& objectPool,
        EffectManager& effectManager);
    void spawnNextPhase(const sf::Vector2f& playerPosition,
        ObjectPoolingManager& objectPool, EffectManager& effectManager);
    void releaseActiveRoomMonsters(ObjectPoolingManager& objectPool);
};
