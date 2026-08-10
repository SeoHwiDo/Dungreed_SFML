#pragma once

#include <memory>
#include <string>
#include <vector>

#include <SFML/Graphics/RenderWindow.hpp>

class ObjectPoolingManager;
class Player;
class Room;
struct RoomTileSet;
class TileMap;

/// 전투 판정과 같은 개발용 시각화만 담당하는 디버그 렌더링 관리자입니다.
class DebugManager {
public:
    DebugManager() = default;
    ~DebugManager();
    /// 현재 플레이어와 활성 몬스터의 피격·공격 판정을 월드 좌표에 맞춰 표시합니다.
    void renderCombatBounds(sf::RenderWindow& window, const Player& player,
        ObjectPoolingManager& objectPool) const;
    /// JSON에서 로드한 방 목록으로 디버그 프리뷰 타일맵을 만들고 화면 안에 자동 배치합니다.
    bool buildRoomPreviews(const std::vector<const Room*>& rooms,
        const std::string& tileAtlasKey, const RoomTileSet& tileSet,
        sf::Vector2u viewportSize);
    /// 생성된 방 프리뷰를 현재 창의 기본 뷰에 렌더링합니다.
    void renderRoomPreviews(sf::RenderWindow& window) const;
    /// 현재 렌더링 가능한 방 프리뷰가 하나 이상 존재하는지 반환합니다.
    bool hasRoomPreviews() const { return !m_roomPreviews.empty(); }

private:
    std::vector<std::unique_ptr<TileMap>> m_roomPreviews;
};
