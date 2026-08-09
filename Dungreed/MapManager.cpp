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
    if (!m_currentRoom) {
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

bool MapManager::buildAllRoomsDebug(const std::string& tileAtlasKey,
    const RoomTileSet& tileSet, sf::Vector2u viewportSize) {
    constexpr std::array<RoomType, 9> roomTypes{
        RoomType::Start, RoomType::Monster, RoomType::Monster2,
        RoomType::Monster3, RoomType::Monster4, RoomType::Monster5,
        RoomType::Trial, RoomType::Hut, RoomType::Boss
    };
    constexpr float previewScale = 0.45f;
    constexpr float margin = 28.f;

    m_rooms.clear();
    float cursorX = margin;
    float cursorY = margin;
    float rowHeight = 0.f;
    for (const RoomType type : roomTypes) {
        auto room = std::make_unique<Room>(type);
        auto preview = std::make_unique<TileMap>();
        if (!room->buildTileMap(*preview, tileAtlasKey, tileSet)) {
            m_rooms.clear();
            return false;
        }
        const sf::Vector2f unscaledSize = preview->getPixelSize();
        const float previewWidth = unscaledSize.x * previewScale;
        const float previewHeight = unscaledSize.y * previewScale;
        if (cursorX + previewWidth > static_cast<float>(viewportSize.x) - margin && cursorX > margin) {
            cursorX = margin;
            cursorY += rowHeight + margin;
            rowHeight = 0.f;
        }
        preview->setPosition({ cursorX, cursorY });
        preview->setScale({ previewScale, previewScale });
        cursorX += previewWidth + margin;
        rowHeight = std::max(rowHeight, previewHeight);
        m_rooms.push_back({ type, std::move(room), std::move(preview) });
    }
    return true;
}

Room* MapManager::getRoom(RoomType type) const {
    for (const ManagedRoom& managedRoom : m_rooms) {
        if (managedRoom.type == type) {
            return managedRoom.room.get();
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

void MapManager::renderAllRoomsDebug(sf::RenderWindow& window) const {
    for (const ManagedRoom& room : m_rooms) {
        window.draw(*room.tileMap);
    }
}
