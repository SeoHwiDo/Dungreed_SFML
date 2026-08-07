#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <SFML/System/Vector2.hpp>
#include "TileMap.h"

enum class RoomType {
    Start,
    Monster,
    Monster2,
    Monster3,
    Monster4,
    Monster5,
    Trial,
    Hut,
    Boss
};

// Values map directly to the numeric room reference data.
enum class RoomCell : std::uint8_t {
    Empty = 0,
    Ceiling = 1,
    Ground = 2,
    LeftWall=3,
    RightWall=4,
    Platform = 5,
    BackTile = 6,
    Door=7,
    SpawnPoint = 8,
    TopLeftCorner = 9,
    TopRightCorner = 10,
    BottomLeftCorner = 11,
    BottomRightCorner = 12,
};

enum class DoorPosition : std::size_t {
    Up = 0,
    Down,
    Left,
    Right
};

using DoorPositions = std::array<bool, 4>;

struct Door {
    sf::Vector2u cell;
    bool isOpen = false;
};

struct RoomLayout {
    unsigned int width = 0;
    unsigned int height = 0;
    std::vector<RoomCell> cells;
};

struct RoomInfo {
    std::vector<Door> doors;
    std::optional<sf::Vector2u> playerSpawnCell;
    DoorPositions doorPositions{ false, false, true, true };
    RoomType type = RoomType::Start;
    bool isClear = false;
    RoomLayout layout;
};

struct RoomTileSet {
    std::string wallTopFrameName;
    std::string wallGroundFrameName;
    std::string wallLeftFrameName;
    std::string wallRightFrameName;
    std::string wallDoorCornerLeftFrameName;
    std::string wallDoorCornerRightFrameName;
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
    std::string platformLeftFrameName;
    std::string platformInnerFrameName;
    std::string platformRightFrameName;
};

class Room {
public:
    explicit Room(RoomType type = RoomType::Start,
        DoorPositions doorPositions = { true, true, true, true });

    // Loads one hardcoded reference-room layout.
    void loadReference(RoomType type, DoorPositions doorPositions = { false, false, true, true });

    // Converts numeric reference data into TileMap render/collision data.
    bool buildTileMap(TileMap& tileMap, const std::string& tileAtlasKey,
        const RoomTileSet& tileSet) const;

    std::optional<sf::Vector2f> getPlayerSpawnPosition(const TileMap& tileMap) const;
    std::optional<sf::Vector2f> getMonsterSpawnPosition(const TileMap& tileMap) const;

    inline const RoomInfo& getInfo() const { return info; }

private:
    RoomInfo info;

    static RoomLayout getReferenceLayout(RoomType type);
};
