#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include "TileMap.h"

class ObjectPoolingManager;
class Player;
class Room;
struct RoomTileSet;
class GameDataManager;
class MapManager;

enum class DebugCommandType { None, SpawnRoom, ToggleCombatBounds, ApplyEasyMode };

struct DebugCommand {
    DebugCommandType type = DebugCommandType::None;
    std::string floorId;
    std::string roomId;
};

/// 전투 판정과 같은 개발용 시각화만 담당하는 디버그 렌더링 관리자입니다.
class DebugManager {
  public:
    static DebugManager &getInstance() {
        static DebugManager instance;
        return instance;
    }

    DebugManager(const DebugManager &) = delete;
    DebugManager &operator=(const DebugManager &) = delete;
    std::optional<DebugCommand> handleEvent(const sf::Event &event,
        const GameDataManager &gameData) const;
    /// F6 메뉴를 콘솔에 출력하고 선택한 디버그 명령을 반환합니다.
    /// 입력을 기다리는 동안 게임 루프는 일시 정지됩니다.
    DebugCommand readConsoleCommand(const GameDataManager &gameData) const;
    /// 현재 플레이어와 활성 몬스터의 피격·공격 판정을 월드 좌표에 맞춰 표시합니다.
    void renderCombatBounds(sf::RenderWindow &window, const Player &player, ObjectPoolingManager &objectPool) const;
    /// JSON에서 로드한 방 목록으로 디버그 프리뷰 타일맵을 만들고 화면 안에 자동 배치합니다.
    bool buildRoomPreviews(const std::vector<const Room *> &rooms, const std::string &tileAtlasKey, const RoomTileSet &tileSet, sf::Vector2u viewportSize);
    /// 생성된 방 프리뷰를 현재 창의 기본 뷰에 렌더링합니다.
    void renderRoomPreviews(sf::RenderWindow &window) const;
    /// 현재 렌더링 가능한 방 프리뷰가 하나 이상 존재하는지 반환합니다.
    bool hasRoomPreviews() const { return !m_roomPreviews.empty(); }
    /// 지정한 층/방 ID로 맵을 다시 구성합니다. 호출자는 플레이어 배치와 몬스터 스폰을
    /// 이어서 처리할 수 있도록 성공 시 현재 방과 타일맵을 MapManager에 설정합니다.
    bool spawnRoom(const std::string &floorId, const std::string &roomId, const GameDataManager &gameData, MapManager &mapManager, const std::string &tileAtlasKey, const RoomTileSet &tileSet) const;

  private:
    DebugManager() = default;
    ~DebugManager();
    std::vector<std::unique_ptr<TileMap>> m_roomPreviews;
};
