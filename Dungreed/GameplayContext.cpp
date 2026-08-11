#include "GameplayContext.h"

#include "Camera.h"
#include "Player.h"
#include "Room.h"
#include "TileMap.h"

GameplayContext::GameplayContext(Player& player, Camera& camera)
    : m_player(player), m_camera(camera) {}

bool GameplayContext::placePlayerAtRoomSpawn(const Room& room, const TileMap& tileMap,
    float zoom) {
    if (const auto spawn = room.getPlayerSpawnPosition(tileMap)) {
        m_player.setPosition(*spawn);
    }
    setCameraForMap(tileMap, zoom);
    return true;
}

void GameplayContext::setCameraForMap(const TileMap& tileMap, float zoom) {
    const sf::FloatRect mapBounds({ 0.f, 0.f }, tileMap.getPixelSize());
    m_camera.setMapBounds(mapBounds);
    m_camera.setZoom(zoom);
    m_camera.update(m_player.getPosition());
}
