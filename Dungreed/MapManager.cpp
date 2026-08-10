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
    if (floor.rooms.find(roomId) == floor.rooms.end() ||
        floor.rooms.find(floor.startRoomId) == floor.rooms.end() ||
        floor.rooms.find(floor.bossRoomId) == floor.rooms.end()) {
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

        FloorManagedRoom managedRoom;
        managedRoom.room = std::make_unique<Room>();
        m_floorRooms.emplace(instanceId, std::move(managedRoom));
    }

    // JSON의 연결 생성 목록 순서를 디버그 미리보기의 표시 순서로 보관합니다.
    std::unordered_set<std::string> orderedRoomIds;
    const auto addPreviewRoom = [&](const std::string& instanceId) {
        if (m_floorRooms.find(instanceId) != m_floorRooms.end() &&
            orderedRoomIds.insert(instanceId).second) {
            m_floorRoomOrder.push_back(instanceId);
        }
    };
    addPreviewRoom(floor.startRoomId);
    for (const std::string& instanceId : floor.shuffleRoomIds) {
        addPreviewRoom(instanceId);
    }
    addPreviewRoom(floor.bossRoomId);
    for (const auto& roomEntry : floor.rooms) {
        addPreviewRoom(roomEntry.first);
    }

    std::vector<std::string> route;
    route.push_back(floor.startRoomId);
    std::unordered_set<std::string> addedRoomIds{ floor.startRoomId, floor.bossRoomId };
    std::vector<std::string> shuffledRoomIds;
    for (const std::string& instanceId : floor.shuffleRoomIds) {
        if (instanceData.find(instanceId) != instanceData.end() &&
            addedRoomIds.insert(instanceId).second) {
            shuffledRoomIds.push_back(instanceId);
        }
    }
    for (const auto& [instanceId, roomData] : floor.rooms) {
        if (addedRoomIds.insert(instanceId).second) {
            shuffledRoomIds.push_back(instanceId);
        }
    }

    std::mt19937 randomEngine(std::random_device{}());
    std::shuffle(shuffledRoomIds.begin(), shuffledRoomIds.end(), randomEngine);
    route.insert(route.end(), shuffledRoomIds.begin(), shuffledRoomIds.end());
    route.push_back(floor.bossRoomId);

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

    for (const auto& [instanceId, managedRoom] : m_floorRooms) {
        const RoomReferenceData& reference = *referenceData.at(instanceId);
        managedRoom.room->loadLayout(reference.type, reference.layout, doorPositions.at(instanceId));
        managedRoom.room->setMonsterSpawns(instanceData.at(instanceId)->monsterSpawns);
        managedRoom.room->setMonsterPhaseConfig(
            instanceData.at(instanceId)->monsterPhaseConfig);
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
    const TileMap* nextTileMap = findFloorTileMap(nextRoom);
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
    const TileMap& tileMap, const sf::Vector2f& playerPosition) {
    if (!m_currentRoom || m_currentRoom->getInfo().isClear) {
        return;
    }
    monsterManager.requestRoomMonsters(*m_currentRoom, tileMap, gameData, objectPool, playerPosition);
}

const TileMap* MapManager::findFloorTileMap(const Room* room) const {
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
