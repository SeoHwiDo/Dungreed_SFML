#pragma once
#include <memory>
#include <string>
#include <vector>
#include <SFML/Graphics/RenderWindow.hpp>
#include "Room.h"

// Owns the room-layout debug previews. It does not affect gameplay collision maps.
class MapManager {
public:
    bool buildAllRoomsDebug(const std::string& tileAtlasKey,
        const RoomTileSet& tileSet, sf::Vector2u viewportSize);

    void renderAllRoomsDebug(sf::RenderWindow& window) const;
    inline bool hasDebugPreview() const { return !m_debugRooms.empty(); }

private:
    struct DebugRoom {
        RoomType type;
        std::unique_ptr<TileMap> tileMap;
    };

    std::vector<DebugRoom> m_debugRooms;
};
