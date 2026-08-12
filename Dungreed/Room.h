#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <SFML/System/Vector2.hpp>
#include "TileMap.h"

enum class RoomType { Start, Monster, Monster2, Monster3, Monster4, Monster5, Trial, Hut, Boss };

// Values map directly to the numeric room reference data.
enum class RoomCell : std::uint8_t {
    Empty = 0,
    Ceiling = 1,
    Ground = 2,
    LeftWall = 3,
    RightWall = 4,
    Platform = 5,
    BackTile = 6,
    Door = 7,
    SpawnPoint = 8,
    TopLeftCorner = 9,
    TopRightCorner = 10,
    BottomLeftCorner = 11,
    BottomRightCorner = 12,
    // 문과 방 벽이 만나는 지점을 레이아웃 단계에서 직접 구분합니다.
    DoorUpLeftCorner = 13,
    DoorUpRightCorner = 14,
    DoorDownLeftCorner = 15,
    DoorDownRightCorner = 16,
    DoorLeftCorner = 17,
    DoorRightCorner = 18,
    // 문 입구 안쪽에서 벽 그림자가 한쪽에만 이어지는 백타일입니다.
    BackDoorTopLeft = 19,
    BackDoorTopRight = 20,
    BackDoorBottomLeft = 21,
    BackDoorBottomRight = 22,
    // 시작마을의 보이지 않는 이동 경계와, 배경만 보이는 열린 공간입니다.
    InvisibleWall = 23,
    OpenSpace = 24,
};

enum class DoorPosition : std::size_t { Up = 0, Down, Left, Right };

using DoorPositions = std::array<bool, 4>;

class Room;

struct Door {
    // MapManager가 소유하는 다음 방 객체를 가리킵니다. 실제 수명은 MapManager가 관리합니다.
    Room *next = nullptr;
    bool isOpen = false;
};

struct RoomLayout {
    unsigned int width = 0;
    unsigned int height = 0;
    std::vector<RoomCell> cells;
    // 타일 종류와 독립적인 플레이어 시작 위치입니다.
    std::optional<sf::Vector2u> playerSpawnCell;
    // room_data.json의 layout 설정을 보관해 통로 생성·감지·문 배치에서 같은 기준을 사용합니다.
    unsigned int outlineWidth = 2;
    unsigned int topBottomPassageWidth = 3;
    unsigned int sidePassageHeight = 3;
};

struct RoomMonsterSpawn {
    std::string monsterId;
    sf::Vector2f positionOffset{};
    float activationDelay = 1.f;
    int phaseIndex = 0;
};

struct RoomMonsterPhaseConfig {
    struct MonsterCount {
        std::string monsterId;
        int count = 0;
    };

    float phaseDelay = 1.2f;
    float activationDelay = 0.9f;
    /// 바깥 배열의 인덱스가 페이즈이며, 내부 항목은 해당 페이즈의 몬스터 종류와 수량입니다.
    std::vector<std::vector<MonsterCount>> monsterPool;

    bool isEnabled() const { return !monsterPool.empty(); }
};

/// 방 클리어 보상에 사용할 요정의 크기입니다. 실제 이미지와 능력치 수치는 room_data.json이 결정합니다.
enum class FairyRewardSize { Small, Medium, Large, ExtraLarge };

/// 전투방을 클리어했을 때 생성할 보상 상자의 구성입니다.
struct RoomClearRewardConfig {
    bool enabled = false;
    FairyRewardSize fairySize = FairyRewardSize::Small;
    float maxHpIncrease = 0.f;
    float powerIncrease = 0.f;
};

struct RoomInfo {
    std::vector<Door> doors;
    std::optional<sf::Vector2u> playerSpawnCell;
    DoorPositions doorPositions{false, false, true, true};
    RoomType type = RoomType::Start;
    bool isClear = false;
    bool traversalLockOverride = false;
    std::vector<RoomMonsterSpawn> monsterSpawns;
    RoomMonsterPhaseConfig monsterPhaseConfig;
    std::vector<RoomMonsterSpawn> encounterMonsters;
    int encounterPhaseCount = 0;
    bool isEncounterInitialized = false;
    RoomClearRewardConfig clearReward;
    bool clearRewardChestOpened = false;
    bool clearRewardCollected = false;
    RoomLayout layout;
    // 방 레퍼런스에 저장한 충돌 없는 비규격 장식 타일 목록입니다.
    std::vector<DecorativeTileConfig> decorations;
    std::vector<BackgroundLayerConfig> backgroundLayers;
};

struct RoomTileSet {
    std::string WallOutterFrameName;
    std::string wallTopFrameName;
    std::string wallGroundFrameName;
    std::string wallLeftFrameName;
    std::string wallRightFrameName;
    // 문 입구 경계가 꺾이는 방향별 전용 벽 타일입니다.
    // 예: 하단 문은 TopLeft/TopRight, 상단 문은 BottomLeft/BottomRight를 사용합니다.
    std::string wallDoorTopLeftFrameName;
    std::string wallDoorTopRightFrameName;
    std::string wallDoorBottomLeftFrameName;
    std::string wallDoorBottomRightFrameName;
    std::string topLeftCornerFrameName;
    std::string topRightCornerFrameName;
    std::string bottomLeftCornerFrameName;
    std::string bottomRightCornerFrameName;
    std::string backInnerFrameName;
    std::string backTopFrameName;
    std::string backGroundFrameName;
    std::string backLeftFrameName;
    std::string backRightFrameName;
    std::string backTopLeftCornerFrameName;
    std::string backTopRightCornerFrameName;
    std::string backBottomLeftCornerFrameName;
    std::string backBottomRightCornerFrameName;
    std::string backDoorTopLeftFrameName;
    std::string backDoorTopRightFrameName;
    std::string backDoorBottomLeftFrameName;
    std::string backDoorBottomRightFrameName;
    // 플랫폼은 길이·양끝 위치와 무관하게 같은 단일 타일을 사용합니다.
    std::string platformFrameName;
};

class Room {
  public:
    /// 방 종류와 연결 문 방향으로 하드코딩된 레퍼런스 레이아웃을 생성합니다.
    explicit Room(RoomType type = RoomType::Start, DoorPositions doorPositions = {true, true, true, true}, std::vector<RoomMonsterSpawn> monsterSpawns = {});

    /// 하드코딩된 레퍼런스 방을 다시 읽고 지정한 방향에만 문 확장 공간을 만듭니다.
    void loadReference(RoomType type, DoorPositions doorPositions = {false, false, true, true});
    void loadLayout(RoomType type, RoomLayout layout, DoorPositions doorPositions, std::vector<DecorativeTileConfig> decorations = {}, std::vector<BackgroundLayerConfig> backgroundLayers = {});
    void setMonsterSpawns(std::vector<RoomMonsterSpawn> monsterSpawns);
    void setMonsterPhaseConfig(RoomMonsterPhaseConfig config);
    /// 방 인스턴스 데이터가 지정한 클리어 보상 구성을 적용합니다.
    void setClearRewardConfig(RoomClearRewardConfig config);
    /// 최초 활성화 때 확정한 몬스터 목록과 페이즈 수를 저장합니다. 빈 목록은 즉시 클리어됩니다.
    void prepareMonsterEncounter(std::vector<RoomMonsterSpawn> monsters, int phaseCount);
    void setClear(bool isClear);
    /// 보스전처럼 몬스터 조우와 별개로 방 이동을 막아야 할 때 사용합니다.
    void setTraversalLocked(bool locked);
    /// 몬스터가 하나라도 남아 있는, 이동을 막아야 하는 전투방인지 반환합니다.
    bool isTraversalLocked() const;
    /// 전투방 클리어 보상 상자의 개방·수령 상태를 보관합니다.
    bool isClearRewardChestOpened() const { return info.clearRewardChestOpened; }
    bool isClearRewardCollected() const { return info.clearRewardCollected; }
    const RoomClearRewardConfig &getClearRewardConfig() const { return info.clearReward; }
    void setClearRewardChestOpened(bool opened) { info.clearRewardChestOpened = opened; }
    void setClearRewardCollected(bool collected) { info.clearRewardCollected = collected; }

    /// RoomCell 레이아웃을 TileConfig로 변환해 타일맵 렌더링 및 충돌 데이터를 생성합니다.
    /// tileSet에는 벽·문·백타일·플랫폼에 대응하는 아틀라스 프레임 이름을 전달합니다.
    bool buildTileMap(TileMap &tileMap, const std::string &tileAtlasKey, const RoomTileSet &tileSet) const;

    /// SpawnPoint 셀을 타일 중앙의 월드 좌표로 변환합니다. 스폰 셀이 없으면 nullopt입니다.
    std::optional<sf::Vector2f> getPlayerSpawnPosition(const TileMap &tileMap) const;
    /// BackTile 중 스폰에 안전한 빈 셀 하나를 선택해 타일 중앙 좌표를 반환합니다. 후보가 없으면 nullopt입니다.
    std::optional<sf::Vector2f> getMonsterSpawnPosition(const TileMap &tileMap) const;
    std::vector<sf::Vector2f> getMonsterSpawnPositions(const TileMap &tileMap) const;
    std::optional<DoorPosition> getEnteredDoor(const sf::FloatRect &actorBounds, const sf::FloatRect &previousActorBounds, const TileMap &tileMap) const;

    /// 현재 방의 레이아웃·문·스폰 정보를 읽기 전용으로 반환합니다.
    inline const RoomInfo &getInfo() const { return info; }
    /// 최초 한 번 확정한 몬스터 목록을 반환합니다.
    inline const std::vector<RoomMonsterSpawn> &getEncounterMonsters() const { return info.encounterMonsters; }
    /// 확정된 전투 페이즈 수를 반환합니다.
    inline int getEncounterPhaseCount() const { return info.encounterPhaseCount; }
    /// 몬스터 목록이 이미 확정되었는지 반환합니다.
    inline bool isMonsterEncounterPrepared() const { return info.isEncounterInitialized; }

    /// Up/Down/Left/Right 방향 슬롯에 연결할 다음 Room 객체를 지정합니다.
    void setDoorNext(DoorPosition position, Room *nextRoom);

    /// 지정한 문 방향의 다음 Room 객체를 반환합니다. 해당 문이나 연결 대상이 없으면 nullptr입니다.
    Room *getDoorNext(DoorPosition position) const;

  private:
    RoomInfo info;

    /// 방 종류에 맞는 기본 벽·백타일·플랫폼 레이아웃을 생성합니다. 문 확장은 loadReference가 적용합니다.
    static RoomLayout getReferenceLayout(RoomType type);
};
