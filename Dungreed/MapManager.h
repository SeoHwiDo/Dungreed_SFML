#pragma once
#include <memory>
#include <string>
#include <vector>
#include <SFML/Graphics/RenderWindow.hpp>
#include "Room.h"

/// 모든 레퍼런스 방을 한 화면에 배치해 확인하는 디버그 미리보기를 소유합니다.
/// 이 클래스가 만드는 TileMap은 실제 게임 플레이 충돌 맵에는 영향을 주지 않습니다.
class MapManager {
public:
    /// 모든 RoomType을 생성해 viewportSize 안에 겹치지 않도록 배치합니다. 디버그 보기 시작 전에 한 번 호출합니다.
    bool buildAllRoomsDebug(const std::string& tileAtlasKey,
        const RoomTileSet& tileSet, sf::Vector2u viewportSize);

    /// 관리 중인 RoomType에 해당하는 실제 Room 객체를 반환합니다. 없으면 nullptr입니다.
    Room* getRoom(RoomType type) const;

    /// 두 방의 문을 서로 연결합니다. 각 방의 Door::next가 상대 Room 객체를 가리키게 됩니다.
    bool connectRooms(RoomType from, DoorPosition fromDoor,
        RoomType to, DoorPosition toDoor);

    /// buildAllRoomsDebug로 만든 방 타일맵을 창에 순서대로 그립니다.
    void renderAllRoomsDebug(sf::RenderWindow& window) const;
    /// 렌더링할 디버그 방이 하나 이상 준비되었는지 반환합니다.
    inline bool hasDebugPreview() const { return !m_rooms.empty(); }

private:
    struct ManagedRoom {
        RoomType type;
        std::unique_ptr<Room> room;
        std::unique_ptr<TileMap> tileMap;
    };

    // unique_ptr로 Room 객체를 소유해 vector 재할당에도 Door의 Room* 주소가 유지됩니다.
    std::vector<ManagedRoom> m_rooms;
};
