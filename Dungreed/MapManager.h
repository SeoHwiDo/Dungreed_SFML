#pragma once

#include "Room.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <vector>
#include <unordered_map>
struct BFSResult
{
    Room* farthestRoom = nullptr;
    std::unordered_map<Room*, std::size_t> distance;
};
class MapManager {
public:
//=============================== Singleton Pattern ==============================
    inline static MapManager& getInstance() {
        static MapManager instance;
        return instance;
    }

    MapManager(const MapManager&) = delete;
    MapManager& operator=(const MapManager&) = delete;
//================================================================================
   
    
    bool genRoom(std::size_t normalRoomCount, const MonsterSpawnConfig& spawnConfig = {});
    bool linkRoom(Room& first, Room& second);


    const std::vector<std::unique_ptr<Room>>& getRooms() const { return m_rooms; }
    Room* getStart() const { return m_start; }
    Room* getBoss() const { return m_boss; }
    //랜덤 함수용 고정 시드
    void setSeed(std::uint32_t seed);

private:
    MapManager();

    bool owns(const Room& room) const;
    Room& createRoom(RoomType type);
    Room* pickRandomConnectableRoom(const std::vector<Room*>& rooms,const Room* exclude=nullptr);
    std::vector<std::unique_ptr<Room>> m_rooms;
    Room* m_start = nullptr;
    Room* m_boss = nullptr;
    std::mt19937 m_random;
    BFSResult bfs(Room* start) const;
};
