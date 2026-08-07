#include "MapManager.h"
#include <algorithm>
#include <array>

bool MapManager::buildAllRoomsDebug(const std::string& tileAtlasKey,
    const RoomTileSet& tileSet, sf::Vector2u viewportSize)
{
    constexpr std::array<RoomType, 9> roomTypes{
        RoomType::Start,
        RoomType::Monster,
        RoomType::Monster2,
        RoomType::Monster3,
        RoomType::Monster4,
        RoomType::Monster5,
        RoomType::Trial,
        RoomType::Hut,
        RoomType::Boss
    };
    constexpr float previewScale = 0.45f;
    constexpr float margin = 28.f;

    m_debugRooms.clear();
    float cursorX = margin;
    float cursorY = margin;
    float rowHeight = 0.f;

    for (const RoomType type : roomTypes) {
        Room room(type);
        auto preview = std::make_unique<TileMap>();
        if (!room.buildTileMap(*preview, tileAtlasKey, tileSet)) {
            m_debugRooms.clear();
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
        m_debugRooms.push_back({ type, std::move(preview) });
    }
    return true;
}

void MapManager::renderAllRoomsDebug(sf::RenderWindow& window) const {
    for (const DebugRoom& room : m_debugRooms) {
        window.draw(*room.tileMap);
    }
}
