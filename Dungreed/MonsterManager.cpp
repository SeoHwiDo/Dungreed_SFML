#include "MonsterManager.h"

#include <algorithm>
#include <iostream>
#include <vector>

#include "Collision.h"
#include "GameDataManager.h"
#include "EffectManager.h"
#include "Player.h"
#include "Room.h"
#include "TileMap.h"

void MonsterManager::requestRoomMonsters(Room& room, const TileMap& tileMap,
    const GameDataManager& gameData, ObjectPoolingManager& objectPool,
    const sf::Vector2f& playerPosition, EffectManager& effectManager) {
    if (&room == m_activeRoom || room.getInfo().isClear) {
        return;
    }

    if (!room.isMonsterEncounterPrepared()) {
        prepareRoomEncounter(room, gameData);
    }
    if (room.getInfo().isClear) {
        return;
    }

    releaseActiveRoomMonsters(objectPool);
    m_activeRoom = &room;
    m_gameData = &gameData;
    m_activeTileMap = &tileMap;

    m_totalPhaseCount = room.getEncounterPhaseCount();
    m_currentPhase = 0;
    // 첫 페이즈는 방 중앙까지 진입한 뒤에만 시작합니다.
    m_isWaitingForMidpoint = true;
}

void MonsterManager::prepareRoomEncounter(Room& room,
    const GameDataManager& gameData) {
    std::vector<RoomMonsterSpawn> encounterMonsters;
    int phaseCount = 1;
    const RoomMonsterPhaseConfig& phaseConfig = room.getInfo().monsterPhaseConfig;
    if (phaseConfig.isEnabled()) {
        phaseCount = static_cast<int>(phaseConfig.monsterPool.size());
        for (int phaseIndex = 0; phaseIndex < phaseCount; ++phaseIndex) {
            for (const RoomMonsterPhaseConfig::MonsterCount& entry :
                phaseConfig.monsterPool[phaseIndex]) {
                const MonsterData* monsterData = gameData.findMonster(entry.monsterId);
                if (!monsterData || !monsterData->enabled) {
                    continue;
                }

                for (int count = 0; count < std::max(entry.count, 0); ++count) {
                    encounterMonsters.push_back({
                        entry.monsterId,
                        monsterData->behavior.isFlying ? sf::Vector2f{ 0.f, -96.f } : sf::Vector2f{},
                        phaseConfig.activationDelay,
                        phaseIndex
                    });
                }
            }
        }
    } else {
        encounterMonsters = room.getInfo().monsterSpawns;
        for (RoomMonsterSpawn& spawn : encounterMonsters) {
            spawn.phaseIndex = 0;
        }
    }

    room.prepareMonsterEncounter(std::move(encounterMonsters), phaseCount);
}

bool MonsterManager::spawnMonster(const MonsterData& monsterData,
    std::vector<sf::Vector2f>& spawnCandidates,
    const sf::Vector2f& positionOffset, float activationDelay,
    const sf::Vector2f& playerPosition,
    int phaseIndex, ObjectPoolingManager& objectPool,
    EffectManager& effectManager) {
    if (!m_activeTileMap) {
        return false;
    }

    Monster* monster = objectPool.acquireMonster(monsterData.id,
        monsterData.status, monsterData.atlasKey, monsterData.behavior);
    monster->setEquipment(m_gameData->createEquip(monsterData.weaponId));

    constexpr float kMinimumSpawnDistance = 48.f;
    constexpr float kPreferredSpawnDistance = 224.f;
    const sf::Vector2f mapSize = m_activeTileMap->getPixelSize();

    // 플레이어 주변의 가까운 바닥부터 검사하되, 즉시 충돌하는 위치는 피합니다.
    std::stable_sort(spawnCandidates.begin(), spawnCandidates.end(),
        [&playerPosition](const sf::Vector2f& left, const sf::Vector2f& right) {
            const sf::Vector2f leftDelta = left - playerPosition;
            const sf::Vector2f rightDelta = right - playerPosition;
            return leftDelta.x * leftDelta.x + leftDelta.y * leftDelta.y <
                rightDelta.x * rightDelta.x + rightDelta.y * rightDelta.y;
        });

    for (std::size_t index = 0; index < spawnCandidates.size(); ++index) {
        const sf::Vector2f spawnPosition =
            spawnCandidates[index] + positionOffset;
        const sf::Vector2f playerDelta = spawnPosition - playerPosition;
        const float distanceSquared = playerDelta.x * playerDelta.x +
            playerDelta.y * playerDelta.y;
        if (distanceSquared < kMinimumSpawnDistance * kMinimumSpawnDistance ||
            distanceSquared > kPreferredSpawnDistance * kPreferredSpawnDistance) {
            continue;
        }

        monster->setPosition(spawnPosition);
        const sf::FloatRect bounds = monster->getGlobalBounds();
        const bool isInsideMap =
            bounds.position.x >= 0.f &&
            bounds.position.y >= 0.f &&
            bounds.position.x + bounds.size.x <= mapSize.x &&
            bounds.position.y + bounds.size.y <= mapSize.y;
        if (!isInsideMap) {
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

        const float revealDelay = effectManager.spawnMonsterMagicCircle(objectPool,
            monster->getBodyCenterPosition());
        monster->beginSpawn(std::max(activationDelay, revealDelay), revealDelay);
        m_activeRoomMonsters.push_back(monster);
        m_monsterPhaseIndices.emplace(monster, phaseIndex);
        spawnCandidates.erase(spawnCandidates.begin() + index);
        return true;
    }

    std::cerr << "[몬스터] 안전한 스폰 위치 없음: "
              << monsterData.id << '\n';
    objectPool.releaseMonster(monster);
    return false;
}

void MonsterManager::spawnNextPhase(
    const sf::Vector2f& playerPosition,
    ObjectPoolingManager& objectPool, EffectManager& effectManager) {
    if (!m_activeRoom || !m_gameData || !m_activeTileMap ||
        m_currentPhase >= m_totalPhaseCount) {
        return;
    }

    const int phaseIndex = m_currentPhase++;
    std::vector<sf::Vector2f> spawnCandidates =
        m_activeRoom->getMonsterSpawnPositions(*m_activeTileMap);
    int requestedCount = 0;
    int spawnedCount = 0;
    for (const RoomMonsterSpawn& spawnInfo :
        m_activeRoom->getEncounterMonsters()) {
        if (spawnInfo.phaseIndex != phaseIndex) {
            continue;
        }
        ++requestedCount;

        const MonsterData* monsterData =
            m_gameData->findMonster(spawnInfo.monsterId);
        if (!monsterData || !monsterData->enabled) {
            continue;
        }
        if (spawnMonster(*monsterData, spawnCandidates,
            spawnInfo.positionOffset, spawnInfo.activationDelay,
            playerPosition, phaseIndex, objectPool, effectManager)) {
            ++spawnedCount;
        }
    }

    std::cout << "[Monster phase] " << (phaseIndex + 1)
              << '/' << m_totalPhaseCount
              << ", requested=" << requestedCount
              << ", spawned=" << spawnedCount << '\n';
}

void MonsterManager::update(float dt, Player& player,
    ObjectPoolingManager& objectPool, const TileMap& tileMap,
    EffectManager& effectManager) {
    if (m_activeRoom && m_isWaitingForMidpoint) {
        if (player.getBodyCenterPosition().x < m_activeTileMap->getPixelSize().x * 0.5f) {
            return;
        }
        m_isWaitingForMidpoint = false;
        spawnNextPhase(player.getBodyCenterPosition(), objectPool, effectManager);
    }

    std::vector<Monster*>& finished = m_finishedMonsters;
    finished.clear();
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
        m_monsterPhaseIndices.erase(monster);
        m_activeRoomMonsters.erase(
            std::remove(m_activeRoomMonsters.begin(),
                m_activeRoomMonsters.end(), monster),
            m_activeRoomMonsters.end());
    }
    finished.clear();

    if (!m_activeRoom) {
        return;
    }

    const int activePhaseIndex = m_currentPhase - 1;
    const int aliveInActivePhase = static_cast<int>(std::count_if(
        m_activeRoomMonsters.begin(), m_activeRoomMonsters.end(),
        [&](Monster* monster) {
            const auto phaseIt = m_monsterPhaseIndices.find(monster);
            return monster && !monster->dead() && phaseIt != m_monsterPhaseIndices.end() &&
                phaseIt->second == activePhaseIndex;
        }));

    if (m_currentPhase < m_totalPhaseCount) {
        // 현재 페이즈에 한 마리만 남으면 다음 페이즈를 즉시 겹쳐 소환합니다.
        if (aliveInActivePhase <= 1) {
            spawnNextPhase(player.getBodyCenterPosition(), objectPool, effectManager);
        }
        return;
    }

    if (!m_activeRoomMonsters.empty()) {
        return;
    }

    m_activeRoom->setClear(true);
    releaseActiveRoomMonsters(objectPool);
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
    m_monsterPhaseIndices.clear();
    m_activeRoom = nullptr;
    m_gameData = nullptr;
    m_activeTileMap = nullptr;
    m_totalPhaseCount = 0;
    m_currentPhase = 0;
    m_isWaitingForMidpoint = false;
}
