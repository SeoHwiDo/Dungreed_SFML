#include "MapManager.h"

#include <algorithm>
#include <array>
#include <random>
#include <unordered_set>
#include <utility>

#include "GameDataManager.h"
#include "MonsterManager.h"
#include "ObjectPoolingManager.h"

namespace {
struct RoomConnection {
    std::string fromId;
    DoorPosition fromDoor;
    std::string toId;
    DoorPosition toDoor;
};

bool selectConnectionDoors(const RoomInstanceData& fromData,
    const RoomInstanceData& toData, DoorPositions& fromDoors,
    DoorPositions& toDoors, DoorPosition& fromDoor, DoorPosition& toDoor) {
    constexpr std::array<std::pair<DoorPosition, DoorPosition>, 4> pairs{
        std::pair{ DoorPosition::Right, DoorPosition::Left },
        std::pair{ DoorPosition::Left, DoorPosition::Right },
        std::pair{ DoorPosition::Down, DoorPosition::Up },
        std::pair{ DoorPosition::Up, DoorPosition::Down }
    };

    for (const auto& [candidateFromDoor, candidateToDoor] : pairs) {
        const std::size_t fromIndex = static_cast<std::size_t>(candidateFromDoor);
        const std::size_t toIndex = static_cast<std::size_t>(candidateToDoor);
        const bool isFromAvailable = std::find(fromData.availableDoorPositions.begin(),
            fromData.availableDoorPositions.end(), candidateFromDoor) !=
            fromData.availableDoorPositions.end();
        const bool isToAvailable = std::find(toData.availableDoorPositions.begin(),
            toData.availableDoorPositions.end(), candidateToDoor) !=
            toData.availableDoorPositions.end();
        if (!fromDoors[fromIndex] && !toDoors[toIndex] &&
            isFromAvailable && isToAvailable) {
            fromDoor = candidateFromDoor;
            toDoor = candidateToDoor;
            fromDoors[fromIndex] = true;
            toDoors[toIndex] = true;
            return true;
        }
    }
    return false;
}

bool selectFixedConnectionDoors(const RoomInstanceData& fromData,
    const RoomInstanceData& toData, DoorPositions& fromDoors,
    DoorPositions& toDoors, DoorPosition fromDoor, DoorPosition toDoor) {
    const std::size_t fromIndex = static_cast<std::size_t>(fromDoor);
    const std::size_t toIndex = static_cast<std::size_t>(toDoor);
    const bool isFromAvailable = std::find(fromData.availableDoorPositions.begin(),
        fromData.availableDoorPositions.end(), fromDoor) !=
        fromData.availableDoorPositions.end();
    const bool isToAvailable = std::find(toData.availableDoorPositions.begin(),
        toData.availableDoorPositions.end(), toDoor) !=
        toData.availableDoorPositions.end();
    if (fromDoors[fromIndex] || toDoors[toIndex] || !isFromAvailable || !isToAvailable) {
        return false;
    }
    fromDoors[fromIndex] = true;
    toDoors[toIndex] = true;
    return true;
}
}

Room& MapManager::createCurrentRoom(RoomType type, DoorPositions doorPositions,
    std::vector<RoomMonsterSpawn> monsterSpawns) {
    m_floorRooms.clear();
    m_floorRoomOrder.clear();
    m_manualRoom = std::make_unique<Room>(type, doorPositions, std::move(monsterSpawns));
    m_currentRoom = m_manualRoom.get();
    m_currentTileMap = nullptr;
    return *m_currentRoom;
}

bool MapManager::createCurrentRoomFromData(const FloorData& floor,
    const std::string& roomId) {
    const bool hasBossRoom = !floor.bossRoomId.empty();
    if (floor.rooms.find(roomId) == floor.rooms.end() ||
        floor.rooms.find(floor.startRoomId) == floor.rooms.end() ||
        (hasBossRoom && floor.rooms.find(floor.bossRoomId) == floor.rooms.end())) {
        return false;
    }

    m_manualRoom.reset();
    m_floorRooms.clear();
    m_floorRoomOrder.clear();
    m_currentRoom = nullptr;
    m_currentTileMap = nullptr;

    std::unordered_map<std::string, const RoomInstanceData*> instanceData;
    std::unordered_map<std::string, const RoomReferenceData*> referenceData;
    std::unordered_map<std::string, DoorPositions> doorPositions;
    for (const auto& [instanceId, roomData] : floor.rooms) {
        const auto referenceIt = floor.roomReferences.find(roomData.roomReferenceId);
        if (referenceIt == floor.roomReferences.end()) {
            m_floorRooms.clear();
            return false;
        }

        instanceData.emplace(instanceId, &roomData);
        referenceData.emplace(instanceId, &referenceIt->second);
        doorPositions.emplace(instanceId, DoorPositions{ false, false, false, false });
    }

    std::mt19937 randomEngine(std::random_device{}());
    std::vector<std::string> monsterRoomCandidates;
    std::vector<std::string> restRoomIds;
    std::vector<std::string> requiredRoomIds;
    std::unordered_set<std::string> classifiedRoomIds{ floor.startRoomId };
    if (hasBossRoom) {
        classifiedRoomIds.insert(floor.bossRoomId);
    }
    const auto classifyRoom = [&](const std::string& instanceId) {
        const auto instanceIt = instanceData.find(instanceId);
        if (instanceIt == instanceData.end() ||
            !classifiedRoomIds.insert(instanceId).second) {
            return;
        }
        if (referenceData.at(instanceId)->type == RoomType::Monster) {
            monsterRoomCandidates.push_back(instanceId);
        } else if (referenceData.at(instanceId)->type == RoomType::Hut) {
            restRoomIds.push_back(instanceId);
        } else {
            requiredRoomIds.push_back(instanceId);
        }
    };
    for (const std::string& instanceId : floor.shuffleRoomIds) {
        classifyRoom(instanceId);
    }
    for (const auto& [instanceId, roomData] : floor.rooms) {
        classifyRoom(instanceId);
    }

    std::shuffle(monsterRoomCandidates.begin(), monsterRoomCandidates.end(), randomEngine);
    const std::size_t monsterRoomCount = floor.randomMonsterRoomCount == 0
        ? monsterRoomCandidates.size()
        : std::min(floor.randomMonsterRoomCount, monsterRoomCandidates.size());
    monsterRoomCandidates.resize(monsterRoomCount);

    std::vector<std::string> intermediateRoomIds = monsterRoomCandidates;
    intermediateRoomIds.insert(intermediateRoomIds.end(),
        requiredRoomIds.begin(), requiredRoomIds.end());

    std::vector<std::string> route;
    route.push_back(floor.startRoomId);
    route.insert(route.end(), intermediateRoomIds.begin(), intermediateRoomIds.end());
    if (hasBossRoom && floor.bossRoomId != floor.startRoomId) {
        route.push_back(floor.bossRoomId);
    }

    for (const std::string& instanceId : route) {
        FloorManagedRoom managedRoom;
        managedRoom.room = std::make_unique<Room>();
        m_floorRooms.emplace(instanceId, std::move(managedRoom));
        m_floorRoomOrder.push_back(instanceId);
    }
    for (const std::string& instanceId : restRoomIds) {
        FloorManagedRoom managedRoom;
        managedRoom.room = std::make_unique<Room>();
        m_floorRooms.emplace(instanceId, std::move(managedRoom));
        m_floorRoomOrder.push_back(instanceId);
    }

    std::vector<RoomConnection> connections;
    for (std::size_t index = 0; index + 1 < route.size(); ++index) {
        const std::string& fromId = route[index];
        const std::string& toId = route[index + 1];
        DoorPosition fromDoor = DoorPosition::Up;
        DoorPosition toDoor = DoorPosition::Down;
        if (!selectConnectionDoors(*instanceData.at(fromId), *instanceData.at(toId),
            doorPositions.at(fromId), doorPositions.at(toId),
            fromDoor, toDoor)) {
            m_floorRooms.clear();
            return false;
        }
        connections.push_back({ fromId, fromDoor, toId, toDoor });
    }

    // 휴식방은 메인 경로의 몬스터방 3개 중 가운데 방 하단에 가지로 연결합니다.
    if (!restRoomIds.empty() && !monsterRoomCandidates.empty()) {
        const std::string& middleMonsterId =
            monsterRoomCandidates[monsterRoomCandidates.size() / 2];
        const std::string& restRoomId = restRoomIds.front();
        if (!selectFixedConnectionDoors(*instanceData.at(middleMonsterId),
            *instanceData.at(restRoomId), doorPositions.at(middleMonsterId),
            doorPositions.at(restRoomId), DoorPosition::Down, DoorPosition::Up)) {
            m_floorRooms.clear();
            return false;
        }
        connections.push_back({ middleMonsterId, DoorPosition::Down,
            restRoomId, DoorPosition::Up });
    }

    for (const auto& [instanceId, managedRoom] : m_floorRooms) {
        const RoomReferenceData& reference = *referenceData.at(instanceId);
        managedRoom.room->loadLayout(reference.type, reference.layout,
            doorPositions.at(instanceId), reference.decorations,
            reference.backgroundLayers);
        managedRoom.room->setMonsterSpawns(instanceData.at(instanceId)->monsterSpawns);
        managedRoom.room->setMonsterPhaseConfig(
            instanceData.at(instanceId)->monsterPhaseConfig);
        managedRoom.room->setClearRewardConfig(
            instanceData.at(instanceId)->clearReward);
    }
    for (const RoomConnection& connection : connections) {
        Room* fromRoom = m_floorRooms.at(connection.fromId).room.get();
        Room* toRoom = m_floorRooms.at(connection.toId).room.get();
        if (fromRoom == toRoom || fromRoom->getDoorNext(connection.fromDoor) ||
            toRoom->getDoorNext(connection.toDoor)) {
            m_floorRooms.clear();
            m_currentRoom = nullptr;
            return false;
        }
        fromRoom->setDoorNext(connection.fromDoor, toRoom);
        toRoom->setDoorNext(connection.toDoor, fromRoom);
    }

    m_currentRoom = m_floorRooms.at(roomId).room.get();
    return true;
}
bool MapManager::preloadFloorTileMaps(const std::string& tileAtlasKey,
    const RoomTileSet& tileSet) {
    if (!m_currentRoom || m_floorRooms.empty()) {
        return false;
    }

    for (auto& [instanceId, managedRoom] : m_floorRooms) {
        auto tileMap = std::make_unique<TileMap>();
        if (!managedRoom.room->buildTileMap(*tileMap, tileAtlasKey, tileSet)) {
            for (auto& [clearInstanceId, clearManagedRoom] : m_floorRooms) {
                clearManagedRoom.tileMap.reset();
            }
            m_currentTileMap = nullptr;
            return false;
        }
        managedRoom.tileMap = std::move(tileMap);
    }

    m_currentTileMap = findFloorTileMap(m_currentRoom);
    return m_currentTileMap != nullptr;
}

bool MapManager::moveCurrentRoom(DoorPosition doorPosition) {
    if (!m_currentRoom || m_currentRoom->isTraversalLocked()) {
        return false;
    }

    Room* nextRoom = m_currentRoom->getDoorNext(doorPosition);
    TileMap* nextTileMap = findFloorTileMap(nextRoom);
    if (!nextRoom || nextRoom == m_currentRoom || !nextTileMap) {
        return false;
    }

    m_currentRoom = nextRoom;
    m_currentTileMap = nextTileMap;
    return true;
}

bool MapManager::buildCurrentRoom(TileMap& tileMap, const std::string& tileAtlasKey,
    const RoomTileSet& tileSet) const {
    return m_currentRoom && m_currentRoom->buildTileMap(tileMap, tileAtlasKey, tileSet);
}

void MapManager::requestCurrentRoomMonsters(MonsterManager& monsterManager,
    ObjectPoolingManager& objectPool, const GameDataManager& gameData,
    const TileMap& tileMap, const sf::Vector2f& playerPosition,
    EffectManager& effectManager) {
    if (!m_currentRoom || m_currentRoom->getInfo().isClear) {
        return;
    }
    monsterManager.requestRoomMonsters(*m_currentRoom, tileMap, gameData,
        objectPool, playerPosition, effectManager);
}

TileMap* MapManager::findFloorTileMap(const Room* room) const {
    if (!room) {
        return nullptr;
    }

    for (const auto& [instanceId, managedRoom] : m_floorRooms) {
        if (managedRoom.room.get() == room) {
            return managedRoom.tileMap.get();
        }
    }
    return nullptr;
}

std::vector<const Room*> MapManager::getFloorRoomsInDataOrder() const {
    std::vector<const Room*> rooms;
    rooms.reserve(m_floorRoomOrder.size());
    for (const std::string& instanceId : m_floorRoomOrder) {
        const auto roomIt = m_floorRooms.find(instanceId);
        if (roomIt != m_floorRooms.end() && roomIt->second.room) {
            rooms.push_back(roomIt->second.room.get());
        }
    }
    return rooms;
}

Room* MapManager::getRoom(RoomType type) const {
    for (const std::string& instanceId : m_floorRoomOrder) {
        const auto roomIt = m_floorRooms.find(instanceId);
        if (roomIt != m_floorRooms.end() && roomIt->second.room &&
            roomIt->second.room->getInfo().type == type) {
            return roomIt->second.room.get();
        }
    }
    return nullptr;
}

bool MapManager::connectRooms(RoomType from, DoorPosition fromDoor,
    RoomType to, DoorPosition toDoor) {
    Room* fromRoom = getRoom(from);
    Room* toRoom = getRoom(to);
    if (!fromRoom || !toRoom) {
        return false;
    }
    fromRoom->setDoorNext(fromDoor, toRoom);
    toRoom->setDoorNext(toDoor, fromRoom);
    return true;
}
