#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <SFML/Graphics/RenderWindow.hpp>

#include "Room.h"
#include "TileMap.h"

class GameDataManager;
struct FloorData;
class MonsterManager;
class ObjectPoolingManager;
class EffectManager;

class MapManager {
public:
    static MapManager& getInstance() {
        static MapManager instance;
        return instance;
    }

    MapManager(const MapManager&) = delete;
    MapManager& operator=(const MapManager&) = delete;
    Room& createCurrentRoom(RoomType type, DoorPositions doorPositions,
        std::vector<RoomMonsterSpawn> monsterSpawns = {});
    bool createCurrentRoomFromData(const FloorData& floor,
        const std::string& roomId);
    bool preloadFloorTileMaps(const std::string& tileAtlasKey,
        const RoomTileSet& tileSet);
    bool moveCurrentRoom(DoorPosition doorPosition);

    bool buildCurrentRoom(TileMap& tileMap, const std::string& tileAtlasKey,
        const RoomTileSet& tileSet) const;
    void requestCurrentRoomMonsters(MonsterManager& monsterManager,
        ObjectPoolingManager& objectPool, const GameDataManager& gameData,
        const TileMap& tileMap, const sf::Vector2f& playerPosition,
        EffectManager& effectManager);
    Room* getCurrentRoom() const { return m_currentRoom; }
    TileMap* getCurrentTileMap() const { return m_currentTileMap; }
    /// JSON 연결 생성 순서에 맞춘 방 목록을 반환합니다. 디버그 프리뷰 등 읽기 전용 소비자가 사용합니다.
    std::vector<const Room*> getFloorRoomsInDataOrder() const;
    Room* getRoom(RoomType type) const;
    bool connectRooms(RoomType from, DoorPosition fromDoor,
        RoomType to, DoorPosition toDoor);

private:
    MapManager() = default;
    ~MapManager() = default;
    struct FloorManagedRoom {
        std::unique_ptr<Room> room;
        std::unique_ptr<TileMap> tileMap;
    };
    TileMap* findFloorTileMap(const Room* room) const;

    std::unique_ptr<Room> m_manualRoom;
    std::unordered_map<std::string, FloorManagedRoom> m_floorRooms;
    Room* m_currentRoom = nullptr;
    TileMap* m_currentTileMap = nullptr;
    std::vector<std::string> m_floorRoomOrder;
};
