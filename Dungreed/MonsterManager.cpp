#include "MonsterManager.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#include "Collision.h"
#include "GameDataManager.h"
#include "Player.h"
#include "Room.h"
#include "TileMap.h"

namespace {
int randomBetween(std::mt19937& engine, int minimum, int maximum) {
    std::uniform_int_distribution<int> distribution(minimum, maximum);
    return distribution(engine);
}
}

void MonsterManager::requestRoomMonsters(Room& room, const TileMap& tileMap,
    const GameDataManager& gameData, ObjectPoolingManager& objectPool,
    const sf::Vector2f& playerPosition) {
    if (&room == m_activeRoom || room.getInfo().isClear) {
        return;
    }

    releaseActiveRoomMonsters(objectPool);
    m_activeRoom = &room;
    m_gameData = &gameData;
    m_activeTileMap = &tileMap;

    const RoomMonsterPhaseConfig& phaseConfig =
        room.getInfo().monsterPhaseConfig;
    m_usesPhaseSpawning = phaseConfig.isEnabled();
    if (m_usesPhaseSpawning) {
        m_totalPhaseCount = randomBetween(m_randomEngine,
            phaseConfig.minPhaseCount, phaseConfig.maxPhaseCount);
        m_currentPhase = 0;
        m_phaseDelayTimer = 0.f;
        spawnNextPhase(playerPosition, objectPool);
        return;
    }

    spawnConfiguredMonsters(playerPosition, objectPool);
}

bool MonsterManager::spawnMonster(const MonsterData& monsterData,
    std::vector<sf::Vector2f>& spawnCandidates,
    const sf::Vector2f& positionOffset, float activationDelay,
    const sf::Vector2f& playerPosition,
    ObjectPoolingManager& objectPool) {
    if (!m_activeTileMap) {
        return false;
    }

    Monster* monster = objectPool.acquireMonster(monsterData.id,
        monsterData.status, monsterData.atlasKey, monsterData.behavior);
    monster->setEquipment(m_gameData->createEquip(monsterData.weaponId));

    constexpr float spawnSafetyMargin = 32.f;
    const float minimumDistance =
        monsterData.behavior.attackRange + spawnSafetyMargin;
    const sf::Vector2f mapSize = m_activeTileMap->getPixelSize();

    std::shuffle(spawnCandidates.begin(), spawnCandidates.end(), m_randomEngine);
    for (std::size_t index = 0; index < spawnCandidates.size(); ++index) {
        const sf::Vector2f spawnPosition =
            spawnCandidates[index] + positionOffset;
        const sf::Vector2f playerDelta = spawnPosition - playerPosition;
        if (playerDelta.x * playerDelta.x + playerDelta.y * playerDelta.y <=
            minimumDistance * minimumDistance) {
            continue;
        }

        monster->setPosition(spawnPosition);
        const sf::FloatRect bounds = monster->getGlobalBounds();
        const bool isInsideMap =
            bounds.position.x >= 0.f &&
            bounds.position.y >= 0.f &&
            bounds.position.x + bounds.size.x <= mapSize.x &&
            bounds.position.y + bounds.size.y <= mapSize.y;
        if (!isInsideMap || monster->isTargetInAttackRange(playerPosition)) {
            continue;
        }

        bool overlapsMonster = false;
        for (const Monster* activeMonster : m_activeRoomMonsters) {
            if (activeMonster && !activeMonster->dead() &&
                activeMonster->getGlobalBounds().findIntersection(bounds)) {
                overlapsMonster = true;
                break;
            }
        }
        if (overlapsMonster) {
            continue;
        }

        monster->beginSpawn(activationDelay);
        m_activeRoomMonsters.push_back(monster);
        spawnCandidates.erase(spawnCandidates.begin() + index);
        m_hasSpawnedRoomMonsters = true;
        return true;
    }

    std::cerr << "[몬스터] 안전한 스폰 위치 없음: "
              << monsterData.id << '\n';
    objectPool.releaseMonster(monster);
    return false;
}

void MonsterManager::spawnConfiguredMonsters(
    const sf::Vector2f& playerPosition,
    ObjectPoolingManager& objectPool) {
    if (!m_activeRoom || !m_gameData || !m_activeTileMap) {
        return;
    }

    std::vector<sf::Vector2f> spawnCandidates =
        m_activeRoom->getMonsterSpawnPositions(*m_activeTileMap);
    if (spawnCandidates.empty()) {
        std::cerr << "[몬스터] 방 스폰 후보 위치 없음\n";
        return;
    }

    for (const RoomMonsterSpawn& spawnInfo :
        m_activeRoom->getInfo().monsterSpawns) {
        const MonsterData* monsterData =
            m_gameData->findMonster(spawnInfo.monsterId);
        if (!monsterData || !monsterData->enabled) {
            std::cerr << "[몬스터] 사용할 수 없는 데이터: "
                      << spawnInfo.monsterId << '\n';
            continue;
        }

        spawnMonster(*monsterData, spawnCandidates,
            spawnInfo.positionOffset, spawnInfo.activationDelay,
            playerPosition, objectPool);
    }
}

void MonsterManager::spawnNextPhase(
    const sf::Vector2f& playerPosition,
    ObjectPoolingManager& objectPool) {
    if (!m_activeRoom || !m_gameData || !m_activeTileMap ||
        m_currentPhase >= m_totalPhaseCount) {
        return;
    }

    const RoomMonsterPhaseConfig& config =
        m_activeRoom->getInfo().monsterPhaseConfig;
    std::vector<sf::Vector2f> spawnCandidates =
        m_activeRoom->getMonsterSpawnPositions(*m_activeTileMap);
    if (spawnCandidates.empty()) {
        std::cerr << "[몬스터 페이즈] 방 스폰 후보 위치 없음\n";
        ++m_currentPhase;
        return;
    }

    const int requestedCount = randomBetween(m_randomEngine,
        config.minMonstersPerPhase, config.maxMonstersPerPhase);
    std::uniform_int_distribution<std::size_t> monsterDistribution(
        0, config.monsterPool.size() - 1);

    ++m_currentPhase;
    int spawnedCount = 0;
    for (int index = 0; index < requestedCount; ++index) {
        const std::string& monsterId =
            config.monsterPool[monsterDistribution(m_randomEngine)];
        const MonsterData* monsterData = m_gameData->findMonster(monsterId);
        if (!monsterData || !monsterData->enabled) {
            std::cerr << "[몬스터 페이즈] 사용할 수 없는 데이터: "
                      << monsterId << '\n';
            continue;
        }

        const sf::Vector2f positionOffset = monsterData->behavior.isFlying
            ? sf::Vector2f{ 0.f, -96.f }
            : sf::Vector2f{};
        if (spawnMonster(*monsterData, spawnCandidates,
            positionOffset, config.activationDelay,
            playerPosition, objectPool)) {
            ++spawnedCount;
        }
    }

    m_phaseDelayTimer = 0.f;
    std::cout << "[몬스터 페이즈] " << m_currentPhase
              << '/' << m_totalPhaseCount
              << ", 요청=" << requestedCount
              << ", 생성=" << spawnedCount << '\n';
}

void MonsterManager::update(float dt, Player& player,
    ObjectPoolingManager& objectPool, const TileMap& tileMap) {
    std::vector<Monster*> finished;
    objectPool.forEachActiveMonster([&](Monster& monster) {
        monster.update(dt, player);
        if (monster.ignoresWalls()) {
            const sf::FloatRect bounds = monster.getGlobalBounds();
            const sf::Vector2f mapSize = tileMap.getPixelSize();
            sf::Vector2f correction{};

            if (bounds.position.x < 0.f) {
                correction.x = -bounds.position.x;
            } else if (bounds.position.x + bounds.size.x > mapSize.x) {
                correction.x = mapSize.x -
                    (bounds.position.x + bounds.size.x);
            }

            if (bounds.position.y < 0.f) {
                correction.y = -bounds.position.y;
            } else if (bounds.position.y + bounds.size.y > mapSize.y) {
                correction.y = mapSize.y -
                    (bounds.position.y + bounds.size.y);
            }

            monster.move(correction.x, correction.y);
        } else {
            Collision::resolveMapCollision(
                monster, tileMap, monster.isFlying());
        }
        if (monster.readyForPoolRelease()) {
            finished.push_back(&monster);
        }
    });

    for (Monster* monster : finished) {
        objectPool.releaseMonster(monster);
        m_activeRoomMonsters.erase(
            std::remove(m_activeRoomMonsters.begin(),
                m_activeRoomMonsters.end(), monster),
            m_activeRoomMonsters.end());
    }

    if (!m_activeRoom || !m_activeRoomMonsters.empty()) {
        return;
    }

    if (m_usesPhaseSpawning) {
        if (m_currentPhase < m_totalPhaseCount) {
            m_phaseDelayTimer += dt;
            if (m_phaseDelayTimer >=
                m_activeRoom->getInfo().monsterPhaseConfig.phaseDelay) {
                spawnNextPhase(player.getBodyCenterPosition(), objectPool);
            }
            return;
        }

        m_activeRoom->setClear(true);
        m_activeRoom = nullptr;
        m_gameData = nullptr;
        m_activeTileMap = nullptr;
        m_hasSpawnedRoomMonsters = false;
        m_usesPhaseSpawning = false;
        return;
    }

    if (m_hasSpawnedRoomMonsters) {
        m_activeRoom->setClear(true);
        m_activeRoom = nullptr;
        m_gameData = nullptr;
        m_activeTileMap = nullptr;
        m_hasSpawnedRoomMonsters = false;
    }
}

void MonsterManager::clearActiveRoom(
    ObjectPoolingManager& objectPool) {
    releaseActiveRoomMonsters(objectPool);
}

void MonsterManager::releaseActiveRoomMonsters(
    ObjectPoolingManager& objectPool) {
    for (Monster* monster : m_activeRoomMonsters) {
        objectPool.releaseMonster(monster);
    }
    m_activeRoomMonsters.clear();
    m_activeRoom = nullptr;
    m_gameData = nullptr;
    m_activeTileMap = nullptr;
    m_hasSpawnedRoomMonsters = false;
    m_usesPhaseSpawning = false;
    m_totalPhaseCount = 0;
    m_currentPhase = 0;
    m_phaseDelayTimer = 0.f;
}
