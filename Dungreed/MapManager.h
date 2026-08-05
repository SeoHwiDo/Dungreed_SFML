#pragma once

#include "Room.h"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <vector>

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

    void minimap(sf::RenderTarget& target, sf::Vector2f origin = { 24.f, 24.f }) const;

    const std::vector<std::unique_ptr<Room>>& getRooms() const { return m_rooms; }
    Room* getStart() const { return m_start; }
    Room* getBoss() const { return m_boss; }
    //랜덤 함수용 고정 시드
    void setSeed(std::uint32_t seed);

private:
    MapManager();

    bool owns(const Room& room) const;
    Room& createRoom(RoomType type);
    sf::Color minimapColor(RoomType type) const;

    std::vector<std::unique_ptr<Room>> m_rooms;
    Room* m_start = nullptr;
    Room* m_boss = nullptr;
    std::mt19937 m_random;
};
