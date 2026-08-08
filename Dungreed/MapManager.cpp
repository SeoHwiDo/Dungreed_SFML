#include "MapManager.h"
#include <algorithm>
#include <array>

/// 모든 레퍼런스 방을 만든 뒤 미리보기 크기에 맞춰 줄바꿈하며 배치합니다. 하나라도 생성 실패하면 전체를 비웁니다.
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

/// 관리 목록에서 RoomType을 찾아 실제 객체 주소를 반환합니다.
Room* MapManager::getRoom(RoomType type) const {
    for (const ManagedRoom& managedRoom : m_rooms) {
        if (managedRoom.type == type) {
            return managedRoom.room.get();
        }
    }
    return nullptr;
}

/// 지정한 두 방향의 Door에 상대 Room 포인터를 기록해 양방향 이동 경로를 만듭니다.
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

/// 저장된 미리보기 타일맵만 그리므로, 게임 오브젝트나 실제 충돌 상태에는 영향을 주지 않습니다.
void MapManager::renderAllRoomsDebug(sf::RenderWindow& window) const {
    for (const ManagedRoom& room : m_rooms) {
        window.draw(*room.tileMap);
    }
}
